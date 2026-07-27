#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// -------------------------------------------------------------
// Read credentials from .env file via load_env.py
// -------------------------------------------------------------
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_NAME"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

const char* ssid        = WIFI_SSID;
const char* password    = WIFI_PASSWORD;

// Fallback Access Point Credentials
const char* ap_ssid     = "ESP32-Button-Network";
const char* ap_password = "password123";


constexpr uint8_t BUTTON_PIN = 18;
constexpr unsigned long DEBOUNCE_MS = 40;

WebServer server(80);

// State tracking
bool lastButtonReading = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
uint32_t pressCount = 0;
bool isPressed = false;

// Modern Dashboard HTML served by ESP32
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Physical Button Dashboard</title>
  <style>
    :root {
      --bg-color: #0f172a;
      --card-bg: #1e293b;
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --accent-active: #22c55e;
      --accent-idle: #475569;
      --border-color: #334155;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; }

    body {
      background-color: var(--bg-color);
      color: var(--text-main);
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      padding: 20px;
    }

    .container {
      background: var(--card-bg);
      border: 1px solid var(--border-color);
      border-radius: 20px;
      padding: 36px 28px;
      max-width: 440px;
      width: 100%;
      box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);
      text-align: center;
    }

    h1 {
      font-size: 1.6rem;
      margin-bottom: 6px;
      font-weight: 700;
      letter-spacing: -0.5px;
    }

    p.subtitle {
      color: var(--text-muted);
      font-size: 0.9rem;
      margin-bottom: 28px;
    }

    .indicator-box {
      width: 150px;
      height: 150px;
      border-radius: 50%;
      margin: 0 auto 28px;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      background: rgba(255, 255, 255, 0.02);
      border: 4px solid var(--accent-idle);
      transition: all 0.15s cubic-bezier(0.4, 0, 0.2, 1);
    }

    .indicator-box.pressed {
      border-color: var(--accent-active);
      box-shadow: 0 0 35px rgba(34, 197, 94, 0.45);
      background: rgba(34, 197, 94, 0.15);
      transform: scale(1.06);
    }

    .status-text {
      font-size: 1.25rem;
      font-weight: 800;
      text-transform: uppercase;
      letter-spacing: 1.5px;
    }

    .stats-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 16px;
      margin-top: 24px;
    }

    .stat-card {
      background: rgba(15, 23, 42, 0.6);
      border: 1px solid var(--border-color);
      border-radius: 14px;
      padding: 18px 12px;
    }

    .stat-value {
      font-size: 2rem;
      font-weight: 800;
      color: var(--text-main);
    }

    .stat-label {
      font-size: 0.75rem;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 0.5px;
      margin-top: 4px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 Button Server</h1>
    <p class="subtitle">Real-Time Hardware Monitor</p>

    <div id="indicator" class="indicator-box">
      <span id="status" class="status-text">RELEASED</span>
    </div>

    <div class="stats-grid">
      <div class="stat-card">
        <div id="count" class="stat-value">0</div>
        <div class="stat-label">Total Presses</div>
      </div>
      <div class="stat-card">
        <div id="uptime" class="stat-value">0s</div>
        <div class="stat-label">Uptime</div>
      </div>
    </div>
  </div>

  <script>
    function updateStatus() {
      fetch('/api/status')
        .then(res => res.json())
        .then(data => {
          const indicator = document.getElementById('indicator');
          const statusText = document.getElementById('status');
          const countText = document.getElementById('count');
          const uptimeText = document.getElementById('uptime');

          if (data.isPressed) {
            indicator.classList.add('pressed');
            statusText.innerText = 'PRESSED';
          } else {
            indicator.classList.remove('pressed');
            statusText.innerText = 'RELEASED';
          }

          countText.innerText = data.pressCount;
          uptimeText.innerText = data.uptimeSec + 's';
        })
        .catch(err => console.error('API Error:', err));
    }

    setInterval(updateStatus, 250);
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", HTML_PAGE);
}

void handleStatusApi() {
  String json = "{";
  json += "\"isPressed\":" + String(isPressed ? "true" : "false") + ",";
  json += "\"pressCount\":" + String(pressCount) + ",";
  json += "\"uptimeSec\":" + String(millis() / 1000);
  json += "}";
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("\nConnecting to Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n===========================================");
    Serial.println("Connected to Home Wi-Fi!");
    Serial.print("Local IP Address: http://");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp32")) {
      Serial.println("mDNS Active! Website URL: http://esp32.local");
    }
    Serial.println("===========================================");
  } else {
    Serial.println("\nHome Wi-Fi connection timed out. Starting Access Point fallback...");
    WiFi.mode(WIFI_AP);
    IPAddress local_ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(ap_ssid, ap_password);

    Serial.println("===========================================");
    Serial.println("Access Point Created!");
    Serial.println("SSID:     ESP32-Button-Network");
    Serial.println("Password: password123");
    Serial.println("URL:      http://192.168.4.1");
    Serial.println("===========================================");
  }

  server.on("/", handleRoot);
  server.on("/api/status", handleStatusApi);
  server.begin();
  Serial.println("Web Server running!");
}

void loop() {
  server.handleClient();

  const bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      isPressed = (buttonState == LOW);

      if (isPressed) {
        pressCount++;
        Serial.print("Button Pressed! Count: ");
        Serial.println(pressCount);
      } else {
        Serial.println("Button Released!");
      }
    }
  }

  lastButtonReading = reading;
}




