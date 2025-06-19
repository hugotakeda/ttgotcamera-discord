#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>

// Configuração do Wi-Fi
const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA_WIFI";

// Configuração do Access Point
const char* ap_ssid = "TTGO-Motion-Detector";
const char* ap_password = "vasscl62";

// Configuração do Webhook do Discord
const char* discordWebhook = "SUA_WEBHOOK_DO_DISCORD";

// Configuração do Servidor Node.js
const char* logServer = "SEU_IP";
const int logServerPort = 3000;

// Configuração do Sensor PIR
#define PIR_PIN 33
#define LED_PIN 2

// Objetos e Variáveis Globais
WiFiClient client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000); // UTC-3 (Brasília)
WebServer server(80);
volatile bool motionDetected = false;
bool apMode = false;
unsigned long lastMotionTime = 0;
const unsigned long motionCooldown = 5000; // 5 segundos entre detecções

void IRAM_ATTR detectsMovement() {
  unsigned long currentTime = millis();
  if (currentTime - lastMotionTime > motionCooldown) {
    motionDetected = true;
    lastMotionTime = currentTime;
  }
}

String getFormattedTime() {
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  time_t localTime = epochTime;
  struct tm* timeinfo = localtime(&localTime);
  
  char date[11];
  char time[9];
  strftime(date, sizeof(date), "%d/%m/%Y", timeinfo);
  strftime(time, sizeof(time), "%H:%M:%S", timeinfo);
  
  return String(date) + " às " + String(time);
}

void sendDiscordNotification(String timestamp) {
  WiFiClientSecure secureClient;
  secureClient.setInsecure();
  
  if (!secureClient.connect("discord.com", 443)) {
    Serial.println("❌ Falha na conexão com o Discord");
    return;
  }

  StaticJsonDocument<512> doc;
  doc["content"] = "";
  JsonArray embeds = doc.createNestedArray("embeds");
  JsonObject embed = embeds.createNestedObject();
  embed["title"] = "🚨 MOVIMENTO DETECTADO!";
  embed["description"] = "[@everyone] Movimento detectado no TTGO T-Camera!\n⏰ **Horário:** " + timestamp;
  embed["color"] = 15158332; // Cor vermelha
  embed["footer"]["text"] = "Sistema de Monitoramento TTGO";
  
  String payload;
  serializeJson(doc, payload);

  String webhookPath = "/api/webhooks/1385051924616708239/Ff4_olAbQ2k9NLy6_Euah8SUIxIhJzBov9HFoxvzgjvjpwWmOF0KSUun8xBIL40MbNSd";
  
  secureClient.println("POST " + webhookPath + " HTTP/1.1");
  secureClient.println("Host: discord.com");
  secureClient.println("Content-Type: application/json");
  secureClient.print("Content-Length: ");
  secureClient.println(payload.length());
  secureClient.println();
  secureClient.print(payload);

  // Aguardar resposta
  unsigned long timeout = millis();
  while (secureClient.connected() && millis() - timeout < 10000) {
    if (secureClient.available()) {
      String line = secureClient.readStringUntil('\n');
      if (line.startsWith("HTTP/1.1 2")) {
        Serial.println("✅ Notificação enviada ao Discord com sucesso!");
        break;
      }
    }
  }
  secureClient.stop();
}

void sendLogToServer(String timestamp) {
  if (!client.connect(logServer, logServerPort)) {
    Serial.println("❌ Falha na conexão com o servidor de logs");
    return;
  }

  StaticJsonDocument<512> doc;
  doc["timestamp"] = timestamp;
  doc["message"] = "Movimento detectado! " + timestamp;
  doc["device"] = "TTGO T-Camera";
  doc["location"] = "Sensor PIR - Pino 33";
  
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

  // Aguardar resposta
  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 5000) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      if (line.startsWith("HTTP/1.1 2")) {
        Serial.println("✅ Log enviado ao servidor com sucesso!");
        break;
      }
    }
  }
  client.stop();
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>TTGO Motion Detector</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .status { padding: 15px; border-radius: 5px; margin: 10px 0; text-align: center; font-weight: bold; }
        .online { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .info { background: #d1ecf1; color: #0c5460; border: 1px solid #bee5eb; }
        h1 { color: #333; text-align: center; }
        .btn { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }
        .btn:hover { background: #0056b3; }
    </style>
</head>
<body>
    <div class='container'>
        <h1>🎥 TTGO Motion Detector</h1>
        <div class='status online'>✅ Sistema Online</div>
        <div class='info'>📡 Modo Access Point Ativo</div>
        <div class='info'>🌐 IP: )" + WiFi.softAPIP().toString() + R"(</div>
        <div class='info'>📊 Acesse os logs em: <a href='http://)" + String(logServer) + R"(:3000' target='_blank'>)" + String(logServer) + R"(:3000</a></div>
        <p><strong>Status:</strong> Monitorando movimentos...</p>
        <p><strong>Último movimento:</strong> <span id='lastMotion'>Aguardando...</span></p>
        <button class='btn' onclick='location.reload()'>🔄 Atualizar</button>
    </div>
    <script>
        setInterval(() => {
            fetch('/status').then(r => r.json()).then(data => {
                document.getElementById('lastMotion').textContent = data.lastMotion || 'Nenhum movimento detectado';
            });
        }, 2000);
    </script>
</body>
</html>
  )";
  server.send(200, "text/html", html);
}

void handleStatus() {
  StaticJsonDocument<200> doc;
  doc["status"] = "online";
  doc["lastMotion"] = lastMotionTime > 0 ? getFormattedTime() : "Nenhum movimento detectado";
  doc["uptime"] = millis() / 1000;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n🚀 Iniciando TTGO Motion Detector...");
  
  // Configurar pinos
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Tentar conectar ao WiFi
  Serial.print("📡 Conectando ao WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectado ao WiFi!");
    Serial.println("🌐 IP: " + WiFi.localIP().toString());
    
    // Inicializar NTP
    timeClient.begin();
    timeClient.update();
    Serial.println("⏰ Horário sincronizado: " + getFormattedTime());
  } else {
    Serial.println("\n❌ Falha na conexão WiFi. Iniciando modo Access Point...");
    apMode = true;
    
    // Configurar Access Point
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("📡 Access Point criado!");
    Serial.println("🌐 SSID: " + String(ap_ssid));
    Serial.println("🔑 Senha: " + String(ap_password));
    Serial.println("🌐 IP: " + WiFi.softAPIP().toString());
  }
  
  // Configurar servidor web
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("🌐 Servidor web iniciado!");
  
  // Configurar interrupção do PIR
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), detectsMovement, RISING);
  Serial.println("👁️ Sensor PIR configurado!");
  
  digitalWrite(LED_PIN, HIGH); // LED indica sistema pronto
  Serial.println("✅ Sistema pronto para detectar movimentos!");
}

void loop() {
  server.handleClient();
  
  if (motionDetected) {
    Serial.println("🚨 MOVIMENTO DETECTADO!");
    
    // Piscar LED
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, LOW);
      delay(100);
      digitalWrite(LED_PIN, HIGH);
      delay(100);
    }
    
    String timestamp = getFormattedTime();
    
    if (!apMode && WiFi.status() == WL_CONNECTED) {
      // Enviar para Discord
      sendDiscordNotification(timestamp);
      delay(1000);
      
      // Enviar para servidor de logs
      sendLogToServer(timestamp);
    } else {
      Serial.println("⚠️ Modo offline - movimento registrado localmente: " + timestamp);
    }
    
    motionDetected = false;
  }
  
  delay(100);
}
