#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define I2C_SDA 21
#define I2C_SCL 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Configurações do Wi-Fi
const char* ssid = "TEF-CF"; // SSID do Access Point
const char* password = "vasscl62"; // Senha do Access Point
const char* sta_ssid = "Takeda"; // SSID da sua rede Wi-Fi
const char* sta_password = "vasscl62"; // Senha da sua rede Wi-Fi

// Configuração do Servidor no Mac
const char* macServer = "192.168.1.4";
const int macServerPort = 8080;

// Webhook do Discord
const char* discordWebhook = "https://discord.com/api/webhooks/1385051924616708239/Ff4_olAbQ2k9NLy6_Euah8SUIxIhJzBov9HFoxvzgjvjpwWmOF0KSUun8xBIL40MbNSd";

// Configuração do Sensor PIR
const int pirPin = 33;

// Servidor Web no ESP32
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  pinMode(pirPin, INPUT);

  // Inicializar OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("❌ Falha ao inicializar OLED");
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Inicializando...");
  display.display();

  // Configurar Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("🌐 AP IP: ");
  Serial.println(IP);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Access Point Ativo");
  display.print("SSID: ");
  display.println(ssid);
  display.print("IP: ");
  display.println(IP);
  display.display();

  // Conectar à rede Wi-Fi
  WiFi.begin(sta_ssid, sta_password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectado ao Wi-Fi!");
    Serial.print("🌐 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Falha ao conectar ao Wi-Fi");
  }

  // Iniciar servidor web
  server.begin();
}

void sendMotionToMac(String timestamp) {
  HTTPClient http;
  String url = String("http://") + macServer + ":" + macServerPort + "/motion";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<512> doc;
  doc["timestamp"] = timestamp;
  doc["message"] = "Movimento detectado!";
  doc["device"] = "TTGO T-Camera";
  doc["location"] = "Sensor PIR - Pino 33";
  
  String payload;
  serializeJson(doc, payload);

  int httpCode = http.POST(payload);
  if (httpCode == 200) {
    Serial.println("✅ Sinal de movimento enviado ao Mac!");
  } else {
    Serial.print("❌ Falha ao enviar ao Mac: ");
    Serial.println(httpCode);
  }
  http.end();
}

void sendDiscordNotification(String timestamp) {
  HTTPClient http;
  http.begin(discordWebhook);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<512> doc;
  doc["content"] = "[@everyone] Movimento detectado!\n⏰ Horário: " + timestamp;
  doc["username"] = "TTGO Motion Detector";

  String payload;
  serializeJson(doc, payload);

  int httpCode = http.POST(payload);
  if (httpCode == 200 || httpCode == 204) {
    Serial.println("✅ Notificação enviada ao Discord!");
  } else {
    Serial.print("❌ Falha ao enviar ao Discord: ");
    Serial.println(httpCode);
  }
  http.end();
}

void handleClient() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("GET /logs") != -1) {
      HTTPClient http;
      String url = String("http://") + macServer + ":" + macServerPort + "/logs";
      http.begin(url);
      int httpCode = http.GET();
      if (httpCode == 200) {
        String payload = http.getString();
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();
        client.println(payload);
      } else {
        client.println("HTTP/1.1 500 Internal Server Error");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Erro ao obter logs do servidor");
      }
      http.end();
    } else {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println("<!DOCTYPE HTML>");
      client.println("<html>");
      client.println("<head><title>TTGO T-Camera Logs</title></head>");
      client.println("<body>");
      client.println("<h1>TTGO T-Camera Logs</h1>");
      client.println("<p>Acesse <a href='http://192.168.1.4:8080'>o site principal</a> para logs completos e gráficos.</p>");
      client.println("</body></html>");
    }
    client.stop();
  }
}

void loop() {
  handleClient();

  int pirState = digitalRead(pirPin);
  if (pirState == HIGH) {
    Serial.println("🚨 MOVIMENTO DETECTADO!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Movimento Detectado!");
    display.display();

    String timestamp = String(millis() / 1000) + "s";
    sendMotionToMac(timestamp);
    sendDiscordNotification(timestamp);

    delay(5000); // Evitar múltiplas detecções rápidas
  }
}
