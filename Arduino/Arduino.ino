#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

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
const char* discordWebhook = "https://discord.com/api/webhooks/1385346002709381343/G1FI6RC7OHd8sJVJCIEqabSqtjB-OTtSnP4UO3vW3jaNach0LmQptjqU-IizDGLZfVrP";

// Configuração do Sensor PIR
const int pirPin = 33;

// Configuração de tempo
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600; // GMT-3 (Brasília)
const int daylightOffset_sec = 0;

// Variáveis de controle
unsigned long lastMotionTime = 0;
const unsigned long motionCooldown = 5000; // 5 segundos entre detecções
int motionCount = 0;

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
  
  showBootScreen();
  
  // Configurar Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("🌐 AP IP: ");
  Serial.println(IP);
  
  showAPInfo(IP);
  delay(2000);

  // Conectar à rede Wi-Fi
  connectToWiFi();
  
  // Configurar NTP para obter horário real
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("⏰ Sincronizando horário...");
  
  // Aguardar sincronização do tempo
  int attempts = 0;
  while (!time(nullptr) && attempts < 10) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (time(nullptr)) {
    Serial.println("✅ Horário sincronizado!");
  } else {
    Serial.println("⚠️ Falha na sincronização do horário");
  }

  // Iniciar servidor web
  server.begin();
  
  showReadyScreen();
  Serial.println("🚀 Sistema TTGO Motion Detector iniciado!");
  Serial.println("📡 Aguardando detecções de movimento...");
}

void showBootScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("TTGO Motion Detector");
  display.println("v2.0");
  display.println("");
  display.println("Inicializando...");
  display.println("Sistemas: OK");
  display.display();
  delay(2000);
}

void showAPInfo(IPAddress IP) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Access Point Ativo");
  display.println("==================");
  display.print("SSID: ");
  display.println(ssid);
  display.print("IP: ");
  display.println(IP);
  display.println("");
  display.println("Conectando WiFi...");
  display.display();
}

void connectToWiFi() {
  WiFi.begin(sta_ssid, sta_password);
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Atualizar display a cada 5 tentativas
    if (attempts % 5 == 0) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Conectando WiFi...");
      display.println("==================");
      display.print("Rede: ");
      display.println(sta_ssid);
      display.print("Tentativa: ");
      display.print(attempts);
      display.println("/30");
      display.display();
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectado ao Wi-Fi!");
    Serial.print("🌐 IP Local: ");
    Serial.println(WiFi.localIP());
    Serial.print("📶 Força do Sinal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Conectado!");
    display.println("===============");
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.print("RSSI: ");
    display.print(WiFi.RSSI());
    display.println(" dBm");
    display.display();
    delay(2000);
  } else {
    Serial.println("\n❌ Falha ao conectar ao Wi-Fi");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi: FALHA");
    display.println("Modo AP apenas");
    display.display();
    delay(2000);
  }
}

void showReadyScreen() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Sistema Pronto!");
  display.println("===============");
  display.println("Status: Online");
  display.print("Deteccoes: ");
  display.println(motionCount);
  display.println("");
  display.println("Aguardando...");
  display.display();
}

String getCurrentTimestamp() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // Se não conseguir obter o horário, usar millis como fallback
    return String(millis() / 1000) + "s (sem sync)";
  }
  
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%d/%m/%Y às %H:%M:%S", &timeinfo);
  return String(timeString);
}

String getDeviceInfo() {
  String info = "TTGO T-Camera ESP32";
  info += " | IP: " + WiFi.localIP().toString();
  info += " | RSSI: " + String(WiFi.RSSI()) + "dBm";
  info += " | Uptime: " + String(millis() / 1000) + "s";
  return info;
}

void sendMotionToMac(String timestamp) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi desconectado - não é possível enviar ao Mac");
    return;
  }
  
  HTTPClient http;
  String url = String("http://") + macServer + ":" + macServerPort + "/motion";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000); // Timeout de 5 segundos

  StaticJsonDocument<1024> doc;
  doc["timestamp"] = timestamp;
  doc["message"] = "🚨 Movimento detectado pelo sensor PIR";
  doc["device"] = "TTGO T-Camera ESP32";
  doc["device_id"] = WiFi.macAddress();
  doc["location"] = "Sensor PIR - GPIO 33";
  doc["signal_strength"] = WiFi.RSSI();
  doc["uptime_seconds"] = millis() / 1000;
  doc["detection_count"] = motionCount;
  doc["local_ip"] = WiFi.localIP().toString();
  doc["firmware_version"] = "2.0";
  
  String payload;
  serializeJson(doc, payload);

  Serial.println("📤 Enviando dados ao servidor Mac...");
  Serial.println("Payload: " + payload);
  
  int httpCode = http.POST(payload);
  if (httpCode == 200) {
    Serial.println("✅ Dados enviados ao Mac com sucesso!");
  } else {
    Serial.print("❌ Falha ao enviar ao Mac - Código HTTP: ");
    Serial.println(httpCode);
    if (httpCode > 0) {
      String response = http.getString();
      Serial.println("Resposta: " + response);
    }
  }
  http.end();
}

void sendDiscordNotification(String timestamp) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi desconectado - não é possível enviar ao Discord");
    return;
  }
  
  HTTPClient http;
  http.begin(discordWebhook);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000); // Timeout de 10 segundos para Discord

  StaticJsonDocument<1024> doc;
  
  // Criar mensagem rica para Discord
  String content = "🚨 **ALERTA DE MOVIMENTO DETECTADO** 🚨\n\n";
  content += "📅 **Data/Hora:** " + timestamp + "\n";
  content += "📍 **Local:** Sensor PIR (GPIO 33)\n";
  content += "🔧 **Dispositivo:** TTGO T-Camera ESP32\n";
  content += "🆔 **MAC:** " + WiFi.macAddress() + "\n";
  content += "📶 **Sinal WiFi:** " + String(WiFi.RSSI()) + " dBm\n";
  content += "⏱️ **Uptime:** " + String(millis() / 1000) + " segundos\n";
  content += "🔢 **Detecção #:** " + String(motionCount) + "\n\n";
  content += "📸 *Foto sendo capturada pelo servidor...*";
  
  doc["content"] = content;
  doc["username"] = "🎥 TTGO Motion Detector";
  doc["avatar_url"] = "https://cdn-icons-png.flaticon.com/512/1001/1001371.png";

  String payload;
  serializeJson(doc, payload);

  Serial.println("📤 Enviando notificação ao Discord...");
  int httpCode = http.POST(payload);
  if (httpCode == 200 || httpCode == 204) {
    Serial.println("✅ Notificação enviada ao Discord com sucesso!");
  } else {
    Serial.print("❌ Falha ao enviar ao Discord - Código HTTP: ");
    Serial.println(httpCode);
    if (httpCode > 0) {
      String response = http.getString();
      Serial.println("Resposta: " + response);
    }
  }
  http.end();
}

void updateDisplay(String status, String timestamp) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("TTGO Motion Detector");
  display.println("===================");
  display.println(status);
  display.println("");
  display.print("Deteccoes: ");
  display.println(motionCount);
  display.println("");
  if (timestamp.length() > 0) {
    display.println("Ultimo:");
    display.println(timestamp.substring(0, 16)); // Mostrar apenas parte do timestamp
  }
  display.display();
}

void handleClient() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("GET /status") != -1) {
      // Endpoint de status do dispositivo
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      
      StaticJsonDocument<512> status;
      status["device"] = "TTGO T-Camera ESP32";
      status["firmware"] = "2.0";
      status["uptime"] = millis() / 1000;
      status["detections"] = motionCount;
      status["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
      status["wifi_rssi"] = WiFi.RSSI();
      status["local_ip"] = WiFi.localIP().toString();
      status["mac_address"] = WiFi.macAddress();
      status["last_motion"] = lastMotionTime;
      
      String response;
      serializeJson(status, response);
      client.println(response);
      
    } else if (request.indexOf("GET /logs") != -1) {
      // Proxy para logs do servidor Mac
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
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();
        client.println("{\"error\":\"Erro ao obter logs do servidor Mac\"}");
      }
      http.end();
      
    } else {
      // Página principal - CORRIGIDO para usar String
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println("<!DOCTYPE HTML>");
      client.println("<html>");
      client.println("<head>");
      client.println("<title>🎥 TTGO Motion Detector</title>");
      client.println("<meta charset='UTF-8'>");
      client.println("<style>body{font-family:Arial;margin:40px;background:#f5f5f5}h1{color:#333}.status{background:#e8f5e8;padding:15px;border-radius:8px;margin:20px 0}.info{background:#fff;padding:15px;border-radius:8px;margin:10px 0;border-left:4px solid #007bff}</style>");
      client.println("</head>");
      client.println("<body>");
      client.println("<h1>🎥 TTGO Motion Detector v2.0</h1>");
      client.println("<div class='status'>");
      client.println("<h3>📊 Status do Sistema</h3>");
      client.println("<p><strong>Dispositivo:</strong> TTGO T-Camera ESP32</p>");
      client.println("<p><strong>Firmware:</strong> v2.0</p>");
      
      // Usar String para concatenação
      String uptimeStr = "<p><strong>Uptime:</strong> " + String(millis() / 1000) + " segundos</p>";
      client.println(uptimeStr);
      
      String detectionsStr = "<p><strong>Detecções:</strong> " + String(motionCount) + "</p>";
      client.println(detectionsStr);
      
      String wifiStatusStr = "<p><strong>WiFi:</strong> " + String(WiFi.status() == WL_CONNECTED ? "Conectado" : "Desconectado") + "</p>";
      client.println(wifiStatusStr);
      
      String ipStr = "<p><strong>IP Local:</strong> " + WiFi.localIP().toString() + "</p>";
      client.println(ipStr);
      
      String macStr = "<p><strong>MAC:</strong> " + WiFi.macAddress() + "</p>";
      client.println(macStr);
      
      client.println("</div>");
      client.println("<div class='info'>");
      client.println("<h3>🔗 Links Úteis</h3>");
      client.println("<p><a href='/status'>📊 Status JSON</a></p>");
      client.println("<p><a href='/logs'>📝 Logs do Servidor</a></p>");
      
      String dashboardLink = "<p><a href='http://" + String(macServer) + ":" + String(macServerPort) + "'>🖥️ Dashboard Principal</a></p>";
      client.println(dashboardLink);
      
      client.println("</div>");
      client.println("<script>setTimeout(function(){location.reload()},30000);</script>");
      client.println("</body></html>");
    }
    client.stop();
  }
}

void loop() {
  handleClient();

  int pirState = digitalRead(pirPin);
  unsigned long currentTime = millis();
  
  if (pirState == HIGH && (currentTime - lastMotionTime) > motionCooldown) {
    motionCount++;
    lastMotionTime = currentTime;
    
    String timestamp = getCurrentTimestamp();
    
    Serial.println("🚨 ================================");
    Serial.println("   MOVIMENTO DETECTADO!");
    Serial.println("🚨 ================================");
    Serial.println("📅 Timestamp: " + timestamp);
    Serial.println("🔢 Detecção #: " + String(motionCount));
    Serial.println("📍 Sensor: PIR GPIO 33");
    Serial.println("🔧 Dispositivo: " + getDeviceInfo());
    Serial.println("================================");
    
    updateDisplay("🚨 MOVIMENTO!", timestamp);
    
    // Enviar notificações
    sendMotionToMac(timestamp);
    delay(500); // Pequeno delay entre envios
    sendDiscordNotification(timestamp);
    
    // Aguardar um pouco antes de voltar ao estado normal
    delay(2000);
    updateDisplay("Aguardando...", "");
    
    Serial.println("✅ Processamento concluído. Aguardando próxima detecção...");
  }
  
  // Atualizar display periodicamente (a cada 30 segundos)
  static unsigned long lastDisplayUpdate = 0;
  if (currentTime - lastDisplayUpdate > 30000) {
    updateDisplay("Sistema Online", "");
    lastDisplayUpdate = currentTime;
  }
  
  delay(100); // Pequeno delay para não sobrecarregar o loop
}
