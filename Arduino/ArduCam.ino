#include <WiFi.h>
#include <WiFiClient.h> // For HTTP communication with Node.js server
#include <WiFiClientSecure.h> // Added for HTTPS communication with Discord
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Configuração do Wi-Fi
const char* ssid = "Takeda";
const char* password = "vasscl62";

// Configuração do Webhook do Discord
const char* discordWebhook = "https://discord.com/api/webhooks/1374892740776689684/qZdTQyp5pv3oK4i1NhB94KBcK1mwf4IWvE-UPKp70w0_4CTLXkslwqyrbNro5dhoWSI8";

// Configuração do Servidor Node.js
const char* logServer = "192.168.1.4"; // Verifique este IP
const int logServerPort = 3000;

// Configuração do Sensor PIR
#define PIR_PIN 33

// Objetos e Variáveis Globais
WiFiClient client; // For Node.js server (HTTP)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
volatile bool motionDetected = false;

void IRAM_ATTR detectsMovement() {
  motionDetected = true;
}

String getFormattedTime() {
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  long offset = -10800;
  time_t localTime = epochTime + offset;
  struct tm* timeinfo = gmtime(&localTime);
  char date[11];
  char time[9];
  strftime(date, sizeof(date), "%d/%m/%Y", timeinfo);
  strftime(time, sizeof(time), "%H:%M:%S", timeinfo);
  return String(date) + " às " + String(time);
}

void sendDiscordNotification(String timestamp) {
  WiFiClientSecure secureClient; // Usar WiFiClientSecure para Discord (HTTPS)
  secureClient.setInsecure(); // Bypass certificate verification (use with caution)
  if (!secureClient.connect("discord.com", 443)) {
    Serial.println("Falha na conexão com o Discord");
    return;
  }

  StaticJsonDocument<512> doc;
  doc["content"] = "";
  JsonArray embeds = doc.createNestedArray("embeds");
  JsonObject embed = embeds.createNestedObject();
  embed["description"] = "[@everyone] Movimento detectado! " + timestamp;
  embed["footer"]["text"] = "Visite a página de logs";
  String payload;
  serializeJson(doc, payload);

  secureClient.println("POST /api/webhooks/1374892740776689684/qZdTQyp5pv3oK4i1NhB94KBcK1mwf4IWvE-UPKp70w0_4CTLXkslwqyrbNro5dhoWSI8 HTTP/1.1");
  secureClient.println("Host: discord.com");
  secureClient.println("Content-Type: application/json");
  secureClient.print("Content-Length: ");
  secureClient.println(payload.length());
  secureClient.println();
  secureClient.print(payload);

  unsigned long timeout = millis();
  while (secureClient.connected() && millis() - timeout < 5000) {
    if (secureClient.available()) {
      String line = secureClient.readStringUntil('\n');
      Serial.println(line);
      if (line == "\r") break;
    }
  }
  secureClient.stop();
  Serial.println("Notificação enviada ao Discord");
}

void sendLogToServer(String timestamp) {
  if (!client.connect(logServer, logServerPort)) {
    Serial.println("Falha na conexão com o servidor de logs");
    return;
  }

  StaticJsonDocument<512> doc;
  doc["timestamp"] = timestamp;
  doc["message"] = "Movimento detectado! " + timestamp;
  String payload;
  serializeJson(doc, payload);

  client.println("POST /log HTTP/1.1");
  client.print("Host: ");
  client.println(logServer);
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(payload.length());
  client.println();
  client.print(payload);

  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 5000) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
      if (line == "\r") break;
    }
  }
  client.stop();
  Serial.println("Log enviado ao servidor");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);

  Serial.print("Conectando ao WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao WiFi");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.setTimeOffset(-10800);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), detectsMovement, RISING);
}

void loop() {
  if (motionDetected) {
    Serial.println("Movimento Detectado!");
    String timestamp = getFormattedTime();
    sendDiscordNotification(timestamp);
    sendLogToServer(timestamp);
    motionDetected = false;
    delay(5000);
  }
  delay(100);
}