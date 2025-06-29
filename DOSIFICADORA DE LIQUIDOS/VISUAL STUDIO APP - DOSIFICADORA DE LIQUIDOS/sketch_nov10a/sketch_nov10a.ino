#include <WiFi.h>
#include <WiFiUdp.h>
#include "esp_system.h"

//------------------- Configuración de LED, WiFi y variables --------------------//
#define ON_Board_LED 2
const char* ssid = "CORONEL"; 
const char* password = "1234asdf"; 

int volumenadosi = 250;
int volumenadosi2 = 250;
int auxposvol = 0;
bool godosificado = false;
float distanciavol = 0;
float distanciavol2 = 0;
float distins = 0;
float Volumen1 = 0;
float Volumen2 = 0;
bool doscompleta1 = false;
bool doscompleta2 = false;

//-------------------- Configuración de pines de Ultrasonido --------------------//
const int trigPin = 16; // US1
const int echoPin = 17;
const int trigPin2 = 32; // US2
const int echoPin2 = 33;
const int trigPin3 = 12; // US3
const int echoPin3 = 13;

#define SOUND_VELOCITY 0.034
long duration;
float distanceCm1, distanceCm2, distanceCm3;

//------------------- Configuración de pines de motores PWM --------------------//
const int in1 = 4;
const int in2 = 5;
int en1 = 18; 
const int pwmfreq = 5000;
const int canal = 0;
const int resolucion = 8;
String aux_lectura;

int velocidad_altura = 0;
#define velocidad_valor 255

int enb1 = 25;
const int canalb = 1;
int velocidad_bomba1 = 0;

int enb2 = 26;
const int canalc = 2;
int velocidad_bomba2 = 0;

//---------------------- Configuración de Timer -----------------------//
volatile boolean Enviar_data = false;
volatile int contador = 0;
int contadorEnvio = 0;
int UStiempo = 1;
const int wdtTimeout = 1000;
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//------------------ Configuración de UDP y WiFi ----------------------//
unsigned int localPort = 8080;
IPAddress DestinationIP(192,168,137,1); 
WiFiUDP udp;

char packetBuffer[50];
String inString;
byte SendBuffer[50];

//------------------- ISR del Timer --------------------//
void IRAM_ATTR OnTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
    contador += 1; 
    if (contador == UStiempo){
      Enviar_data = true;
      contador = 0; 
      contadorEnvio += 1;
    }
  portEXIT_CRITICAL_ISR(&timerMux);
}

//--------------------- Setup ----------------------//
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(trigPin3, OUTPUT);
  pinMode(echoPin3, INPUT);

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(en1, OUTPUT);

  ledcSetup(canal, pwmfreq, resolucion);
  ledcAttachPin(en1, canal);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);

  pinMode(enb1, OUTPUT);
  ledcSetup(canalb, pwmfreq, resolucion);
  ledcAttachPin(enb1, canalb);

  pinMode(enb2, OUTPUT);
  ledcSetup(canalc, pwmfreq, resolucion);
  ledcAttachPin(enb2, canalc);

  WiFi.begin(ssid, password);
  pinMode(ON_Board_LED, OUTPUT);
  digitalWrite(ON_Board_LED, HIGH);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(ON_Board_LED, LOW);
    delay(250);
    digitalWrite(ON_Board_LED, HIGH);
    delay(250);
  }

  digitalWrite(ON_Board_LED, HIGH);
  Serial.println("\nConnected to: " + String(ssid));
  Serial.println("IP Address: " + WiFi.localIP().toString());

  udp.begin(localPort);
  Serial.println("Local Port: " + String(localPort));

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &OnTimer, true);
  timerAlarmWrite(timer, wdtTimeout * 1000, true);
  timerAlarmEnable(timer);
}

//--------------------- Loop ----------------------//
void loop() {
  receive_packet();

  // Medir distancia solo cada 1 segundo
  if (Enviar_data) {
    distanceCm1 = medirDistancia(trigPin, echoPin);
   // Serial.println("tk 1:");
    //Serial.println(distanceCm1);
    distanceCm2 = medirDistancia(trigPin2, echoPin2);
   // Serial.println("tk 2:");
   // Serial.println(distanceCm2);
    distanceCm3 = medirDistancia(trigPin3, echoPin3);
    
    Enviar_data = false;
  }

  // Enviar datos cada 3 segundos
  if (contadorEnvio >= 3) {
    senseUS_and_send_packet();
    contadorEnvio = 0;
  }

  // Control de dosificación y altura
  ledcWrite(canal, velocidad_altura);
  ledcWrite(canalb, velocidad_bomba1);
  ledcWrite(canalc, velocidad_bomba2);

  if (godosificado) {
    if (distanceCm3 < 18 && distanceCm3 > 4) {
      //Serial.println("Cinta detenida");
      //Serial.println("Motores encendidos");
      if ( volumenadosi > 0 && doscompleta1 == false){
          velocidad_bomba1 = 255;
      }
      if ( volumenadosi2 > 0 && doscompleta2 == false){
          velocidad_bomba2 = 255;
      }
      
      if (distanciavol <= distanceCm1) {
        velocidad_bomba1 = 0;
        Serial.println("Bomba 1 detenida");
        doscompleta1 = true;
      }
      if (distanciavol2 <= distanceCm2) {
        velocidad_bomba2 = 0;
        Serial.println("Bomba 2 detenida");
        doscompleta2 = true;
      }
      if (doscompleta1 && doscompleta2) {
        doscompleta1 = doscompleta2 = godosificado = false;
        Serial.println("Dosificación completa");
      }
    }
  }
}

//------------------- Funciones de Ultrasonido y UDP -------------------//
void receive_packet() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;
    aux_lectura = String(packetBuffer);

    if (aux_lectura == "subir") {
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW); 
      velocidad_altura = velocidad_valor;
    } else if (aux_lectura == "bajar") {
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH); 
      velocidad_altura = velocidad_valor; 
    } else if (aux_lectura == "stop") {
      velocidad_altura = velocidad_bomba1 = velocidad_bomba2 = 0;
      doscompleta1 = false;
      doscompleta2 = false;
      godosificado = false;
    } else if (aux_lectura.startsWith("play")) {
      auxposvol = aux_lectura.indexOf("B");
      volumenadosi = aux_lectura.substring(4, auxposvol+1).toInt();
      volumenadosi2 = aux_lectura.substring(auxposvol+1).toInt();
      godosificado = true;

      distanciavol = ((volumenadosi / (3.14 * (5.5 * 5.5)))) + distanceCm1;
      distanciavol2 = ((volumenadosi2 / (3.14 * (5.5 * 5.5)))) + distanceCm2;
      Serial.printf("Distancia vol 1 : %f",distanciavol);
      Serial.printf("Distancia vol 2 : %f",distanciavol2);
    }
  }
}

void senseUS_and_send_packet() {
  Volumen1 = (37.7 - distanceCm1) * (3.14 * (5.5 * 5.5));
  Volumen2 = (37.7 - distanceCm2) * (3.14 * (5.5 * 5.5));
  inString = String(Volumen1) + "B" + String(Volumen2) + "C";
  inString.getBytes(SendBuffer, 50);
  udp.beginPacket(DestinationIP, 8080);
  udp.write(SendBuffer, 50);
  udp.endPacket();
}

float medirDistancia(int trigIndex, int ecoindex) {

  float suma = 0;

  for (int i=0;i<10;i++) {
  digitalWrite(trigIndex, LOW);
  delayMicroseconds(2);
  digitalWrite(trigIndex, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigIndex, LOW);
  duration = pulseIn(ecoindex, HIGH);

  suma = suma +( duration * SOUND_VELOCITY / 2);
  delay(30);
  }
  return suma/10;
 
}
