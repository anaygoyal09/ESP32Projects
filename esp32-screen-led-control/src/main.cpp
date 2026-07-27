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
const char* ap_ssid     = "ESP32-LED-Network";
const char* ap_password = "password123";

// Hardware LED Pin Configurations
constexpr uint8_t ONBOARD_LED_PIN  = 2; // Internal Onboard LED
constexpr uint8_t EXTERNAL_LED_PIN = 4; // Pin P4 on NodeMCU-32 board

WebServer server(80);

// Global LED State Tracking
bool ledState = false;
uint8_t ledBrightness = 255; // 0 to 255

// Update physical LED pin outputs based on state and brightness
void applyLedState() {
  if (ledState) {
    analogWrite(ONBOARD_LED_PIN, ledBrightness);
    analogWrite(EXTERNAL_LED_PIN, ledBrightness);
  } else {
    analogWrite(ONBOARD_LED_PIN, 0);
    analogWrite(EXTERNAL_LED_PIN, 0);
  }
}

// -------------------------------------------------------------
// Web Dashboard HTML served directly from ESP32 flash memory
// -------------------------------------------------------------
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Interactive LED Controller</title>
  <style>
    :root {
      --bg-color: #0b0f19;
      --card-bg: rgba(30, 41, 59, 0.75);
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --glow-on: #f59e0b;
      --glow-on-bg: rgba(245, 158, 11, 0.25);
      --border-color: rgba(255, 255, 255, 0.1);
    }

    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; }

    body {
      background: radial-gradient(circle at top, #1e1b4b 0%, var(--bg-color) 70%);
      color: var(--text-main);
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      padding: 20px;
    }

    .container {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid var(--border-color);
      border-radius: 28px;
      padding: 40px 32px;
      max-width: 440px;
      width: 100%;
      box-shadow: 0 25px 60px -15px rgba(0, 0, 0, 0.7);
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
      font-size: 0.88rem;
      margin-bottom: 32px;
    }

    /* Screen Light Bulb Graphic */
    .bulb-wrapper {
      position: relative;
      width: 170px;
      height: 170px;
      margin: 0 auto 32px;
      cursor: pointer;
      user-select: none;
      border-radius: 50%;
      display: flex;
      justify-content: center;
      align-items: center;
      background: rgba(255, 255, 255, 0.03);
      border: 3px solid rgba(255, 255, 255, 0.08);
      transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    }

    .bulb-wrapper:hover {
      transform: scale(1.05);
      border-color: rgba(255, 255, 255, 0.2);
    }

    .bulb-wrapper:active {
      transform: scale(0.95);
    }

    .bulb-svg {
      width: 80px;
      height: 80px;
      fill: none;
      stroke: #64748b;
      stroke-width: 1.8;
      stroke-linecap: round;
      stroke-linejoin: round;
      transition: all 0.3s ease;
    }

    /* ON State Styling */
    .bulb-wrapper.on {
      border-color: var(--glow-on);
      background: var(--glow-on-bg);
      box-shadow: 0 0 50px rgba(245, 158, 11, 0.5), inset 0 0 25px rgba(245, 158, 11, 0.3);
    }

    .bulb-wrapper.on .bulb-svg {
      stroke: #fbbf24;
      fill: rgba(251, 191, 36, 0.85);
      filter: drop-shadow(0 0 12px #f59e0b);
    }

    .status-badge {
      display: inline-block;
      padding: 6px 18px;
      border-radius: 20px;
      font-size: 0.85rem;
      font-weight: 700;
      letter-spacing: 1px;
      text-transform: uppercase;
      margin-bottom: 28px;
      background: rgba(255, 255, 255, 0.06);
      color: var(--text-muted);
      border: 1px solid var(--border-color);
      transition: all 0.3s ease;
    }

    .status-badge.on {
      background: rgba(245, 158, 11, 0.2);
      color: #fbbf24;
      border-color: rgba(245, 158, 11, 0.4);
    }

    /* Controls Grid */
    .btn-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
      margin-bottom: 28px;
    }

    .btn {
      background: rgba(255, 255, 255, 0.06);
      border: 1px solid var(--border-color);
      color: var(--text-main);
      padding: 14px;
      border-radius: 14px;
      font-weight: 600;
      font-size: 0.9rem;
      cursor: pointer;
      transition: all 0.2s ease;
    }

    .btn:hover {
      background: rgba(255, 255, 255, 0.12);
      transform: translateY(-2px);
    }

    .btn:active {
      transform: translateY(0);
    }

    .btn-on {
      background: rgba(34, 197, 94, 0.15);
      color: #4ade80;
      border-color: rgba(34, 197, 94, 0.3);
    }
    .btn-on:hover { background: rgba(34, 197, 94, 0.28); }

    .btn-off {
      background: rgba(239, 68, 68, 0.15);
      color: #f87171;
      border-color: rgba(239, 68, 68, 0.3);
    }
    .btn-off:hover { background: rgba(239, 68, 68, 0.28); }

    /* Slider Container */
    .slider-section {
      background: rgba(15, 23, 42, 0.6);
      border: 1px solid var(--border-color);
      border-radius: 16px;
      padding: 18px 16px;
      margin-bottom: 24px;
      text-align: left;
    }

    .slider-header {
      display: flex;
      justify-content: space-between;
      font-size: 0.8rem;
      color: var(--text-muted);
      margin-bottom: 10px;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }

    input[type=range] {
      -webkit-appearance: none;
      width: 100%;
      background: #334155;
      height: 8px;
      border-radius: 4px;
      outline: none;
    }

    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 22px;
      height: 22px;
      border-radius: 50%;
      background: var(--glow-on);
      cursor: pointer;
      box-shadow: 0 0 10px var(--glow-on);
    }

    /* Telemetry Stats */
    .stats-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }

    .stat-card {
      background: rgba(15, 23, 42, 0.5);
      border: 1px solid var(--border-color);
      border-radius: 14px;
      padding: 14px 10px;
    }

    .stat-val {
      font-size: 1.1rem;
      font-weight: 700;
      color: var(--text-main);
    }

    .stat-lbl {
      font-size: 0.72rem;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 0.5px;
      margin-top: 4px;
    }
  </style>
</head>
<body>

  <div class="container">
    <h1>ESP32 Screen Control</h1>
    <p class="subtitle">Click light bulb on screen to turn physical LED on</p>

    <!-- Clickable Screen Light Bulb -->
    <div id="bulb" class="bulb-wrapper" onclick="toggleLed()">
      <svg class="bulb-svg" viewBox="0 0 24 24">
        <path d="M9 18h6m-4 3h2M12 2a7 7 0 0 0-7 7c0 2.38 1.19 4.47 3 5.74V17a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1v-2.26c1.81-1.27 3-3.36 3-5.74a7 7 0 0 0-7-7z"/>
      </svg>
    </div>

    <div id="statusBadge" class="status-badge">LED OFF</div>

    <!-- Quick Action Buttons -->
    <div class="btn-grid">
      <button class="btn btn-on" onclick="setLedState(true)">Turn ON</button>
      <button class="btn btn-off" onclick="setLedState(false)">Turn OFF</button>
      <button class="btn" onclick="toggleLed()">Toggle</button>
      <button class="btn" onclick="triggerBlink()">Pulse / Blink</button>
    </div>

    <!-- Brightness Intensity Slider -->
    <div class="slider-section">
      <div class="slider-header">
        <span>LED Brightness</span>
        <span id="sliderVal">100%</span>
      </div>
      <input type="range" id="brightnessSlider" min="0" max="255" value="255" oninput="onBrightnessInput(this.value)">
    </div>

    <!-- Telemetry Stats Grid -->
    <div class="stats-grid">
      <div class="stat-card">
        <div id="rssiVal" class="stat-val">-- dBm</div>
        <div class="stat-lbl">Wi-Fi Signal</div>
      </div>
      <div class="stat-card">
        <div id="uptimeVal" class="stat-val">0s</div>
        <div class="stat-lbl">Uptime</div>
      </div>
    </div>
  </div>

  <script>
    let isFetching = false;

    function updateUI(data) {
      const bulb = document.getElementById('bulb');
      const statusBadge = document.getElementById('statusBadge');
      const sliderVal = document.getElementById('sliderVal');
      const brightnessSlider = document.getElementById('brightnessSlider');
      const rssiVal = document.getElementById('rssiVal');
      const uptimeVal = document.getElementById('uptimeVal');

      if (data.ledState) {
        bulb.classList.add('on');
        statusBadge.classList.add('on');
        statusBadge.innerText = 'LED ON';
      } else {
        bulb.classList.remove('on');
        statusBadge.classList.remove('on');
        statusBadge.innerText = 'LED OFF';
      }

      const percent = Math.round((data.brightness / 255) * 100);
      sliderVal.innerText = percent + '%';
      
      if (document.activeElement !== brightnessSlider) {
        brightnessSlider.value = data.brightness;
      }

      rssiVal.innerText = data.rssi + ' dBm';
      uptimeVal.innerText = data.uptimeSec + 's';
    }

    function fetchStatus() {
      if (isFetching) return;
      isFetching = true;

      fetch('/api/status')
        .then(res => res.json())
        .then(data => {
          updateUI(data);
          isFetching = false;
        })
        .catch(err => {
          console.error('API Error:', err);
          isFetching = false;
        });
    }

    function toggleLed() {
      fetch('/api/led/toggle')
        .then(res => res.json())
        .then(data => updateUI(data))
        .catch(err => console.error(err));
    }

    function setLedState(on) {
      const endpoint = on ? '/api/led/on' : '/api/led/off';
      fetch(endpoint)
        .then(res => res.json())
        .then(data => updateUI(data))
        .catch(err => console.error(err));
    }

    function onBrightnessInput(val) {
      document.getElementById('sliderVal').innerText = Math.round((val / 255) * 100) + '%';
      fetch('/api/led/set?brightness=' + val)
        .then(res => res.json())
        .then(data => updateUI(data))
        .catch(err => console.error(err));
    }

    function triggerBlink() {
      fetch('/api/led/blink')
        .then(res => res.json())
        .then(data => updateUI(data))
        .catch(err => console.error(err));
    }

    setInterval(fetchStatus, 400);
    fetchStatus();
  </script>
</body>
</html>
)rawliteral";

// -------------------------------------------------------------
// WebServer HTTP Route Handlers
// -------------------------------------------------------------

void handleRoot() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", HTML_PAGE);
}

void sendStatusJson() {
  String json = "{";
  json += "\"ledState\":" + String(ledState ? "true" : "false") + ",";
  json += "\"brightness\":" + String(ledBrightness) + ",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"uptimeSec\":" + String(millis() / 1000);
  json += "}";
  
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleStatus() {
  sendStatusJson();
}

void handleLedOn() {
  ledState = true;
  applyLedState();
  sendStatusJson();
}

void handleLedOff() {
  ledState = false;
  applyLedState();
  sendStatusJson();
}

void handleLedToggle() {
  ledState = !ledState;
  applyLedState();
  sendStatusJson();
}

void handleLedSet() {
  if (server.hasArg("brightness")) {
    int val = server.arg("brightness").toInt();
    if (val < 0) val = 0;
    if (val > 255) val = 255;
    ledBrightness = (uint8_t)val;
    if (val > 0) ledState = true;
    else ledState = false;
    applyLedState();
  }
  sendStatusJson();
}

void handleLedBlink() {
  for (int i = 0; i < 3; i++) {
    analogWrite(ONBOARD_LED_PIN, 255);
    analogWrite(EXTERNAL_LED_PIN, 255);
    delay(120);
    analogWrite(ONBOARD_LED_PIN, 0);
    analogWrite(EXTERNAL_LED_PIN, 0);
    delay(120);
  }
  ledState = true;
  applyLedState();
  sendStatusJson();
}

void handleNotFound() {
  server.sendHeader("Connection", "close");
  server.send(404, "text/plain", "404 Not Found");
}

// -------------------------------------------------------------
// Arduino Setup & Loop
// -------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(ONBOARD_LED_PIN, OUTPUT);
  pinMode(EXTERNAL_LED_PIN, OUTPUT);
  applyLedState();

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
      MDNS.addService("http", "tcp", 80);
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
    Serial.println("SSID:     ESP32-LED-Network");
    Serial.println("Password: password123");
    Serial.println("URL:      http://192.168.4.1");
    Serial.println("===========================================");
  }

  // Setup Server Endpoint Handlers
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/led/on", HTTP_GET, handleLedOn);
  server.on("/api/led/off", HTTP_GET, handleLedOff);
  server.on("/api/led/toggle", HTTP_GET, handleLedToggle);
  server.on("/api/led/set", HTTP_GET, handleLedSet);
  server.on("/api/led/blink", HTTP_GET, handleLedBlink);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Web Server running!");
}

void loop() {
  server.handleClient();
}
