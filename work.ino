/*
   Projeto: Célula 3 – Detecção de Vazamento de Gás (Sala Técnica)
   Dispositivo único: ESP8266 (NodeMCU ou similar)
   Funções:
     - Lê o sensor analógico A0 (0–1023)
     - Controla LEDs (verde, amarelo, vermelho)
     - Ativa buzzer em emergência
     - Publica telemetria JSON no MQTT HiveMQ Cloud
     - Publica estado online/offline com LWT
     - Faz reconexão automática de Wi-Fi e MQTT
     - Responde a comandos MQTT ("getStatus")
*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

/* ======== CONFIG Wi-Fi ======== */
const char ssid[] = SECRET_SSID;
const char password[] = SECRET_PASS;

/* ======== CONFIG MQTT ======== */
const char broker[] = SECRET_BROKER;
const int  port = SECRET_PORT;
const char mqttUser[] = SECRET_USER;
const char mqttPassword[] = SECRET_PASS_MQTT;

/* ======== IDENTIDADE ======== */
const int   cellId = 3;
const char* devId  = "c3-gustavo-ramon";
const char* campus = "riodosul";
const char* curso  = "si";
const char* turma  = "BSN22025T26F8";

/* ======== TÓPICOS PADRÃO ======== */
String base = String("iot/") + campus + "/" + curso + "/" + turma +
              "/cell/" + String(cellId) + "/device/" + devId + "/";
String topicState     = base + "state";
String topicTelemetry = base + "telemetry";
String topicCmd       = base + "cmd";

/* ======== HARDWARE ======== */
const int PIN_LED_VERDE    = 4;   // GPIO4 (D2)
const int PIN_LED_AMARELO  = 5;   // GPIO5 (D1)
const int PIN_LED_VERMELHO = 12;  // GPIO12 (D6)
const int PIN_BUZZER       = 13;  // GPIO13 (D7)
const int PIN_SENSOR       = A0;  // ADC0

/* ======== LIMITES ======== */
int limiarAtencao = 300;
int limiarCritico = 600;

/* ======== CLIENTES ======== */
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

/* ======== VARIÁVEIS ======== */
unsigned long ultimoEnvio = 0;
String estadoAtual = "normal";
int ultimoPPM = 0;

/* ======== PROTÓTIPOS ======== */
void connectWiFi();
void connectMQTT();
void publishTelemetry(int ppm, String status);
String classificar(int ppm);
void autoTeste();
void mqttCallback(char* topic, byte* payload, unsigned int length);

/* =======================================================
   SETUP
   ======================================================= */
void setup() {
  Serial.begin(9600);
  delay(200);

  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AMARELO, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_SENSOR, INPUT);

  autoTeste();
  connectWiFi();

  wifiClient.setInsecure();
  mqttClient.setServer(broker, port);
  mqttClient.setCallback(mqttCallback);  // <- callback ativado

  connectMQTT();
}

/* =======================================================
   LOOP PRINCIPAL
   ======================================================= */
void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  int leitura = analogRead(PIN_SENSOR);
  int ppm = map(leitura, 0, 1023, 0, 1000);
  String novoEstado = classificar(ppm);

  // Controle de LEDs e buzzer
  digitalWrite(PIN_LED_VERDE,    novoEstado == "normal" ? HIGH : LOW);
  digitalWrite(PIN_LED_AMARELO,  novoEstado == "atencao" ? HIGH : LOW);
  digitalWrite(PIN_LED_VERMELHO, novoEstado == "critico" ? HIGH : LOW);
  digitalWrite(PIN_BUZZER,       novoEstado == "critico" ? HIGH : LOW);

  // Publica quando o estado muda
  if (novoEstado != estadoAtual) {
    estadoAtual = novoEstado;
    publishTelemetry(ppm, estadoAtual);
  }

  // Publica a cada 3 segundos
  if (millis() - ultimoEnvio > 3000) {
    ultimoEnvio = millis();
    publishTelemetry(ppm, estadoAtual);
  }

  ultimoPPM = ppm;
  delay(200);
}

/* =======================================================
   CONECTAR Wi-Fi
   ======================================================= */
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Conectando Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao conectar Wi-Fi. Reiniciando...");
    delay(3000);
    ESP.restart();
  }
}

/* =======================================================
   CONECTAR MQTT
   ======================================================= */
void connectMQTT() {
  Serial.print("Conectando ao broker MQTT...");
  while (!mqttClient.connected()) {
    if (mqttClient.connect(devId, mqttUser, mqttPassword,
                           topicState.c_str(), 1, true, "offline")) {
      Serial.println("\nConectado ao broker MQTT!");
      mqttClient.publish(topicState.c_str(), "online", true);

      // Inscreve no tópico de comandos
      mqttClient.subscribe(topicCmd.c_str());
      Serial.println("Inscrito em: " + topicCmd);
    } else {
      Serial.print("Falha. Código: ");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

/* =======================================================
   CALLBACK MQTT
   ======================================================= */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();

  Serial.print("Comando recebido em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  if (msg.equalsIgnoreCase("getStatus")) {
    // Envia status atual
    publishTelemetry(ultimoPPM, estadoAtual);
    Serial.println("Status enviado via MQTT.");
  }
}

/* =======================================================
   PUBLICAR TELEMETRIA JSON
   ======================================================= */
void publishTelemetry(int ppm, String status) {
  StaticJsonDocument<300> doc;
  doc["ts"] = millis() / 1000;
  doc["cellId"] = cellId;
  doc["devId"] = devId;

  JsonObject metrics = doc.createNestedObject("metrics");
  metrics["ppm"] = ppm;

  doc["status"] = status;

  JsonObject thr = doc.createNestedObject("thresholds");
  thr["atencao"] = limiarAtencao;
  thr["critico"] = limiarCritico;

  String payload;
  serializeJson(doc, payload);

  mqttClient.publish(topicTelemetry.c_str(), payload.c_str());
  Serial.println("Telemetria enviada: " + payload);
}

/* =======================================================
   CLASSIFICADOR
   ======================================================= */
String classificar(int ppm) {
  if (ppm < limiarAtencao) return "normal";
  if (ppm <= limiarCritico) return "atencao";
  return "critico";
}

/* =======================================================
   TESTE INICIAL LED/BUZZER
   ======================================================= */
void autoTeste() {
  digitalWrite(PIN_LED_VERDE, HIGH);   delay(300);
  digitalWrite(PIN_LED_VERDE, LOW);
  digitalWrite(PIN_LED_AMARELO, HIGH); delay(300);
  digitalWrite(PIN_LED_AMARELO, LOW);
  digitalWrite(PIN_LED_VERMELHO, HIGH);delay(300);
  digitalWrite(PIN_LED_VERMELHO, LOW);
  digitalWrite(PIN_BUZZER, HIGH);      delay(300);
  digitalWrite(PIN_BUZZER, LOW);
}
