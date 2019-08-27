#include <LiquidCrystal.h>
#include <WiFiEsp.h>
#include "Wire.h"
#include "EmonLib.h"
#define DS1307_ADDRESS 0x68
EnergyMonitor SensorAmp;
byte zero = 0x00;
bool estadoSensor1;
bool estadoSensor2;
bool estadoSensor3;
bool estadoSensor4;
bool estadoSensor5;
bool estadoSensor6;
bool estadoSensor7;
int quant = 0;
int i;
int segundos;
int minutos;
int min2;
int horas;
int SCT = A0;
int status;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); //define as portas da tela LCD
// Configuracao de conexao
char ssid[] = "CEEAC";
char pass[] = "c33@ufac";
// -------------------------
char server[] = "medidor.ceeac.org";
WiFiEspClient client;

void Mostrahoras()
{
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ADDRESS, 7);
  segundos = BCDtoDecimal(Wire.read());
  minutos = BCDtoDecimal(Wire.read());
  horas = BCDtoDecimal(Wire.read() & 0b111111);
}

void setup() {
  SensorAmp.current(SCT, 60.606); //Calibracao do sensor de corrente
  lcd.begin(16, 2);
  Serial.begin(115200);
  Serial1.begin(115200);
  Wire.begin();
  pinMode(26, OUTPUT); //saida do rele 1
  pinMode(27, OUTPUT); //saida do rele 2
  pinMode(30, INPUT); //entrada do sensor 1
  pinMode(32, INPUT); //entrada do sensor 2
  pinMode(34, INPUT); //entrada do sensor 3
  pinMode(36, INPUT); //entrada do sensor 4
  pinMode(38, INPUT); //entrada do sensor 5
  pinMode(40, INPUT); //entrada do sensor 6
  pinMode(42, INPUT); //entrada do sensor 7
  digitalWrite(26, HIGH); //Inicia com o rele 1 desligado
  digitalWrite(27, HIGH); //Inicia com o rele 2 desligado
  setupWiFi();
  Mostrahoras(); //Atualiza as horas
  min2 = minutos;
}
void loop() {
  double Irms = SensorAmp.calcIrms(1480); //Calcula o valor da corrente
  estadoSensor1 = digitalRead(30);
  estadoSensor2 = digitalRead(32);
  estadoSensor3 = digitalRead(34);
  estadoSensor4 = digitalRead(36);
  estadoSensor5 = digitalRead(38);
  estadoSensor6 = digitalRead(40);
  estadoSensor7 = digitalRead(42);

  //caso o sensor 1 seja o primeiro a ser detectado
  if ( estadoSensor1 == HIGH ) {
    for (i = 0; i < 25; i++) { //checa o sensor 2 por cinco segundos
      delay(200);
      estadoSensor2 = digitalRead(32);
      if ( estadoSensor2 == HIGH) { //
        quant++; //incrementa o numero de pessoas
        delay(3000);
        break;
      }
    }
  }
  // caso o sensor 2 seja o primeiro a ser detectado
  if ( estadoSensor2 == HIGH) {
    for (i = 0; i < 25; i++) { //checa o sensor 1 por cinco segundos
      delay(200);
      estadoSensor1 = digitalRead(30);
      if ( estadoSensor1 == HIGH) {
        quant--;  //decrementa o numero de pessoas
        delay(3000);
        break;
      }
    }
  }

  Mostrahoras(); //Atualiza as horas

  if (estadoSensor3 == HIGH && estadoSensor4 == HIGH && estadoSensor5 == HIGH && estadoSensor6 == HIGH && estadoSensor7 == HIGH) {
    min2 = minutos;
  }

  if (min2 >= 50) {
    min2 = minutos;
  }

  if (min2 <= minutos + 10) {
    quant--;
    min2 = minutos;
  }

  // impede que a quantidade de pessoas fique negativa.
  if (quant < 0) {
    quant = 0;
  }

  // aciona o rele a partir da quantidade de pessoas na sala
  if (quant == 0 && estadoSensor3 == LOW && estadoSensor4 == LOW && estadoSensor5 == LOW && estadoSensor6 == LOW && estadoSensor7 == LOW) {
    digitalWrite(26, HIGH);
    if ( horas < 6 || horas > 21 ) {
      digitalWrite(27, HIGH);
    }
  } else {
    digitalWrite(26, LOW);
    digitalWrite(27, LOW);
  }

  // escreve no LCD a quantidade de pessoas contadas.
  lcd.clear();
  lcd.setCursor(0, 0);
  if (quant > 0) {
    lcd.print("local com gente");
  } else {
    lcd.print("local vazio");
  }
  Serial.print("Quantidade = ");
  Serial.print(quant);
  lcd.setCursor(0, 1);
  lcd.print("P=");
  lcd.print(Irms * 127);
  lcd.print("W");
  lcd.print(" ");
  lcd.print(horas);
  lcd.print(":");
  lcd.print(minutos);
  //lcd.print(":");
  //lcd.print(segundos);

  if (minutos % 15 == 0) {
    if (status != WL_CONNECTED) {
      setupWiFi();
    }
    if (status == WL_CONNECTED) {
      EnviarDados(127, Irms, 127 * Irms); // 'a', 'b' e 'c' devem ser float
    }
  }

}

byte BCDtoDecimal(byte var)  { //Converte de BCD para decimal
  return ( (var / 16 * 10) + (var % 16) );
}

void setupWiFi()
{
  //Serial onde esta o ESP-01 com o firmware AT ja instalado
  WiFi.init(&Serial1);

  Serial.print("Conectando a ");
  Serial.println(ssid);

  status = WL_IDLE_STATUS;

  for (int i = 0; i < 5; i++) {
    status = WiFi.begin(ssid, pass);
    if (status == WL_CONNECTED) {
      Serial.println();
      Serial.println("Conectado");

      IPAddress localIP = WiFi.localIP();
      Serial.print("IP: ");
      Serial.println(localIP);
      break;
    }

    else {
      Serial.println("Nao conectado");
    }

    delay(1000);
  }
}

void EnviarDados(float tensao, float corrente, float potencia) {
  // Converte os argumentos de inteiros para strings (necessario para realizar o GET)
  String t = String(tensao);
  String c = String(corrente);
  String p = String(tensao * corrente);
  Serial.println("Inicializando requisicao HTTP.");
  if (client.connect(server, 80)) {
    Serial.println("Conectado ao servidor.");
    String url = "/index.php/Inserir/medicao/" + t + "/" + c + "/" + p;
    client.println("GET " + url + " HTTP/1.1");
    client.println("Host: medidor.ceeac.org");
    client.println("Connection: close");
    client.println();
    client.stop();
  }
  Serial.println("Requisicao finalizada.");
}
