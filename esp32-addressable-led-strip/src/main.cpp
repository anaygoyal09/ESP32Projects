#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>

// -------------------------------------------------------------
// Read Wi-Fi credentials from .env via load_env.py
// -------------------------------------------------------------
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_NAME"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Fallback Access Point Credentials
const char* ap_ssid     = "ESP32-LED-Strip";
const char* ap_password = "password123";

// Hardware configuration
constexpr uint8_t LED_DATA_PIN = 5;
constexpr uint16_t MAX_LEDS    = 300; // Maximum supported pixels in memory

// NeoPixel Strip Instance
Adafruit_NeoPixel strip(MAX_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

// Global LED State
bool powerOn            = true;
uint8_t currentRed      = 0;
uint8_t currentGreen    = 180;
uint8_t currentBlue     = 255;
uint8_t currentBrightness = 24; // Safe low-brightness startup for USB testing
uint16_t activeLedCount = 60; // Default active LED count (user adjustable)
String currentMode      = "solid"; // "solid", "rainbow", "breathing", "wipe", "chase", "twinkle", "off"
uint16_t animSpeed      = 40;  // Milliseconds step delay

// Web Server on port 80
WebServer server(80);

// Non-blocking animation timing
unsigned long lastAnimUpdate = 0;
uint16_t animStep = 0;
int breatheDirection = 1;
uint8_t breatheVal = 30;

// Modern Dashboard HTML
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 LED Controller - esp32.local</title>
  <style>
    :root {
      --bg: #0b0f19;
      --card-bg: rgba(30, 41, 59, 0.7);
      --card-border: rgba(255, 255, 255, 0.08);
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --accent: #3b82f6;
      --accent-glow: rgba(59, 130, 246, 0.4);
      --success: #10b981;
      --danger: #ef4444;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Inter', system-ui, -apple-system, sans-serif; }

    body {
      background-color: var(--bg);
      background-image: 
        radial-gradient(at 0% 0%, rgba(59, 130, 246, 0.12) 0px, transparent 50%),
        radial-gradient(at 100% 100%, rgba(139, 92, 246, 0.12) 0px, transparent 50%);
      color: var(--text-main);
      min-height: 100vh;
      padding: 24px 16px;
      display: flex;
      justify-content: center;
      align-items: flex-start;
    }

    .dashboard {
      width: 100%;
      max-width: 540px;
      display: flex;
      flex-direction: column;
      gap: 20px;
    }

    .header-card {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      border: 1px solid var(--card-border);
      border-radius: 20px;
      padding: 20px 24px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.4);
    }

    .header-title h1 {
      font-size: 1.4rem;
      font-weight: 700;
      letter-spacing: -0.5px;
    }

    .header-title p {
      font-size: 0.82rem;
      color: var(--text-muted);
      margin-top: 2px;
    }

    .power-btn {
      width: 52px;
      height: 52px;
      border-radius: 50%;
      border: none;
      background: #334155;
      color: #94a3b8;
      cursor: pointer;
      display: flex;
      justify-content: center;
      align-items: center;
      transition: all 0.2s ease;
      box-shadow: 0 4px 12px rgba(0,0,0,0.3);
    }

    .power-btn svg { width: 24px; height: 24px; fill: currentColor; }

    .power-btn.on {
      background: var(--success);
      color: #ffffff;
      box-shadow: 0 0 20px rgba(16, 185, 129, 0.6);
      transform: scale(1.05);
    }

    .card {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      border: 1px solid var(--card-border);
      border-radius: 20px;
      padding: 22px;
      box-shadow: 0 15px 30px -10px rgba(0, 0, 0, 0.3);
    }

    .card-title {
      font-size: 0.95rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.8px;
      color: var(--text-muted);
      margin-bottom: 16px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    /* Color Control */
    .color-picker-row {
      display: flex;
      align-items: center;
      gap: 16px;
      margin-bottom: 18px;
    }

    .color-input-wrapper {
      position: relative;
      width: 64px;
      height: 64px;
      border-radius: 16px;
      overflow: hidden;
      border: 2px solid var(--card-border);
      box-shadow: 0 8px 16px rgba(0,0,0,0.3);
      cursor: pointer;
    }

    .color-input-wrapper input[type="color"] {
      position: absolute;
      top: -10px;
      left: -10px;
      width: 84px;
      height: 84px;
      border: none;
      cursor: pointer;
      background: none;
    }

    .color-preview-box {
      flex: 1;
      height: 64px;
      border-radius: 16px;
      display: flex;
      flex-direction: column;
      justify-content: center;
      padding: 0 18px;
      background: rgba(15, 23, 42, 0.6);
      border: 1px solid var(--card-border);
    }

    .color-hex { font-size: 1.1rem; font-weight: 700; letter-spacing: 1px; }
    .color-rgb { font-size: 0.8rem; color: var(--text-muted); }

    .presets-grid {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 10px;
    }

    .preset-btn {
      height: 38px;
      border-radius: 10px;
      border: 1px solid rgba(255,255,255,0.15);
      cursor: pointer;
      transition: transform 0.15s ease, box-shadow 0.15s ease;
    }
    .preset-btn:hover { transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.4); }

    /* Sliders */
    .control-group { margin-bottom: 18px; }
    .control-group:last-child { margin-bottom: 0; }

    .control-header {
      display: flex;
      justify-content: space-between;
      font-size: 0.88rem;
      margin-bottom: 8px;
    }
    .control-value { font-weight: 700; color: var(--accent); }

    input[type="range"] {
      width: 100%;
      -webkit-appearance: none;
      appearance: none;
      height: 8px;
      border-radius: 4px;
      background: #334155;
      outline: none;
    }
    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 22px;
      height: 22px;
      border-radius: 50%;
      background: var(--text-main);
      cursor: pointer;
      box-shadow: 0 0 10px var(--accent);
      transition: transform 0.1s ease;
    }
    input[type="range"]::-webkit-slider-thumb:hover { transform: scale(1.15); }

    /* Mode Buttons */
    .modes-grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 10px;
    }

    .mode-btn {
      background: rgba(15, 23, 42, 0.7);
      border: 1px solid var(--card-border);
      border-radius: 12px;
      padding: 12px 8px;
      color: var(--text-main);
      font-size: 0.85rem;
      font-weight: 600;
      cursor: pointer;
      text-align: center;
      transition: all 0.2s ease;
    }
    .mode-btn:hover { background: rgba(51, 65, 85, 0.7); }
    .mode-btn.active {
      background: var(--accent);
      border-color: var(--accent);
      box-shadow: 0 0 16px var(--accent-glow);
    }

    /* Preset LED Count Quick Pills */
    .count-pills {
      display: flex;
      gap: 8px;
      margin-top: 10px;
      flex-wrap: wrap;
    }

    .pill {
      background: rgba(15, 23, 42, 0.7);
      border: 1px solid var(--card-border);
      border-radius: 20px;
      padding: 4px 12px;
      font-size: 0.78rem;
      color: var(--text-muted);
      cursor: pointer;
      transition: all 0.15s ease;
    }
    .pill:hover, .pill.active {
      background: var(--accent);
      color: #fff;
      border-color: var(--accent);
    }

    /* Footer info */
    .info-footer {
      font-size: 0.8rem;
      color: var(--text-muted);
      text-align: center;
      display: flex;
      justify-content: center;
      gap: 16px;
      margin-top: 4px;
    }
  </style>
</head>
<body>
  <div class="dashboard">
    <!-- Header -->
    <div class="header-card">
      <div class="header-title">
        <h1>ESP32 LED Controller</h1>
        <p id="host-label">http://esp32.local</p>
      </div>
      <button id="power-toggle" class="power-btn on" onclick="togglePower()">
        <svg viewBox="0 0 24 24"><path d="M13 3h-2v10h2V3zm4.83 2.17l-1.42 1.42A7.92 7.92 0 0 1 19 12c0 3.87-3.13 7-7 7s-7-3.13-7-7c0-2.05.88-3.9 2.29-5.18L5.87 5.4 5.17 4.7A9.94 9.94 0 0 0 2 12c0 5.52 4.48 10 10 10s10-4.48 10-10c0-2.76-1.12-5.26-2.93-7.07l-.24-.24z"/></svg>
      </button>
    </div>

    <!-- Active LEDs & Brightness -->
    <div class="card">
      <div class="card-title">Strip Configuration</div>
      
      <div class="control-group">
        <div class="control-header">
          <span>Active LED Count</span>
          <span class="control-value"><span id="val-count">60</span> / 300 LEDs</span>
        </div>
        <input type="range" id="slider-count" min="1" max="300" value="60" oninput="updateCountValue(this.value)" onchange="sendControl()">
        <div class="count-pills">
          <span class="pill" onclick="setCountPill(1)">1 LED</span>
          <span class="pill" onclick="setCountPill(10)">10</span>
          <span class="pill" onclick="setCountPill(30)">30</span>
          <span class="pill" onclick="setCountPill(60)">60</span>
          <span class="pill" onclick="setCountPill(120)">120</span>
          <span class="pill" onclick="setCountPill(300)">300 Max</span>
        </div>
      </div>

      <div class="control-group" style="margin-top: 18px;">
        <div class="control-header">
          <span>Brightness</span>
          <span class="control-value" id="val-bright">9%</span>
        </div>
        <input type="range" id="slider-bright" min="0" max="255" value="24" oninput="updateBrightValue(this.value)" onchange="sendControl()">
      </div>
    </div>

    <!-- Color Palette -->
    <div class="card">
      <div class="card-title">Color Selection</div>
      
      <div class="color-picker-row">
        <div class="color-input-wrapper">
          <input type="color" id="color-picker" value="#00b4ff" onchange="onColorPickerChange(this.value)">
        </div>
        <div class="color-preview-box">
          <span class="color-hex" id="val-hex">#00B4FF</span>
          <span class="color-rgb" id="val-rgb">RGB(0, 180, 255)</span>
        </div>
      </div>

      <div class="presets-grid">
        <button class="preset-btn" style="background:#ff3b30;" onclick="setPresetColor(255,0,0)"></button>
        <button class="preset-btn" style="background:#34c759;" onclick="setPresetColor(0,255,0)"></button>
        <button class="preset-btn" style="background:#007aff;" onclick="setPresetColor(0,122,255)"></button>
        <button class="preset-btn" style="background:#af52de;" onclick="setPresetColor(175,82,222)"></button>
        <button class="preset-btn" style="background:#ff9500;" onclick="setPresetColor(255,149,0)"></button>
        <button class="preset-btn" style="background:#5ac8fa;" onclick="setPresetColor(90,200,250)"></button>
        <button class="preset-btn" style="background:#ffcc00;" onclick="setPresetColor(255,204,0)"></button>
        <button class="preset-btn" style="background:#ffffff;" onclick="setPresetColor(255,255,255)"></button>
      </div>
    </div>

    <!-- Lighting Modes -->
    <div class="card">
      <div class="card-title">Lighting Mode</div>
      
      <div class="modes-grid">
        <button class="mode-btn active" id="mode-solid" onclick="setMode('solid')">Solid Color</button>
        <button class="mode-btn" id="mode-rainbow" onclick="setMode('rainbow')">Rainbow Wave</button>
        <button class="mode-btn" id="mode-breathing" onclick="setMode('breathing')">Breathing</button>
        <button class="mode-btn" id="mode-wipe" onclick="setMode('wipe')">Color Wipe</button>
        <button class="mode-btn" id="mode-chase" onclick="setMode('chase')">Color Chase</button>
        <button class="mode-btn" id="mode-twinkle" onclick="setMode('twinkle')">Twinkle</button>
      </div>

      <div class="control-group" style="margin-top: 18px;">
        <div class="control-header">
          <span>Effect Speed</span>
          <span class="control-value" id="val-speed">40ms</span>
        </div>
        <input type="range" id="slider-speed" min="10" max="200" value="40" oninput="document.getElementById('val-speed').innerText=this.value+'ms'" onchange="sendControl()">
      </div>
    </div>

    <!-- Footer Info -->
    <div class="info-footer">
      <span>IP: <strong id="ip-address">--</strong></span>
      <span>•</span>
      <span>Uptime: <strong id="uptime">0s</strong></span>
    </div>
  </div>

  <script>
    let state = {
      power: true,
      red: 0,
      green: 180,
      blue: 255,
      brightness: 24,
      count: 60,
      mode: 'solid',
      speed: 40
    };

    function fetchStatus() {
      fetch('/api/status')
        .then(r => r.json())
        .then(data => {
          state = data;
          syncUI();
        })
        .catch(e => console.error('Status fetch error:', e));
    }

    function syncUI() {
      // Power button
      const powerBtn = document.getElementById('power-toggle');
      if (state.power) {
        powerBtn.classList.add('on');
      } else {
        powerBtn.classList.remove('on');
      }

      // Sliders
      document.getElementById('slider-count').value = state.count;
      document.getElementById('val-count').innerText = state.count;
      
      document.getElementById('slider-bright').value = state.brightness;
      document.getElementById('val-bright').innerText = Math.round((state.brightness/255)*100) + '%';
      
      document.getElementById('slider-speed').value = state.speed;
      document.getElementById('val-speed').innerText = state.speed + 'ms';

      // Colors
      const hex = rgbToHex(state.red, state.green, state.blue);
      document.getElementById('color-picker').value = hex;
      document.getElementById('val-hex').innerText = hex.toUpperCase();
      document.getElementById('val-rgb').innerText = `RGB(${state.red}, ${state.green}, ${state.blue})`;

      // Mode
      document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
      const activeBtn = document.getElementById('mode-' + state.mode);
      if (activeBtn) activeBtn.classList.add('active');

      // Info
      if (state.ip) document.getElementById('ip-address').innerText = state.ip;
      if (state.uptimeSec !== undefined) document.getElementById('uptime').innerText = state.uptimeSec + 's';
    }

    function sendControl() {
      const params = new URLSearchParams({
        power: state.power ? '1' : '0',
        r: state.red,
        g: state.green,
        b: state.blue,
        brightness: state.brightness,
        count: state.count,
        mode: state.mode,
        speed: state.speed
      });

      fetch('/api/control?' + params.toString(), { method: 'POST' })
        .then(r => r.json())
        .then(data => {
          state = data;
          syncUI();
        })
        .catch(e => console.error('Control error:', e));
    }

    function togglePower() {
      state.power = !state.power;
      sendControl();
    }

    function updateCountValue(v) {
      state.count = parseInt(v);
      document.getElementById('val-count').innerText = v;
    }

    function setCountPill(val) {
      state.count = val;
      document.getElementById('slider-count').value = val;
      document.getElementById('val-count').innerText = val;
      sendControl();
    }

    function updateBrightValue(v) {
      state.brightness = parseInt(v);
      document.getElementById('val-bright').innerText = Math.round((v/255)*100) + '%';
    }

    function onColorPickerChange(hexVal) {
      const rgb = hexToRgb(hexVal);
      state.red = rgb.r;
      state.green = rgb.g;
      state.blue = rgb.b;
      sendControl();
    }

    function setPresetColor(r, g, b) {
      state.red = r;
      state.green = g;
      state.blue = b;
      sendControl();
    }

    function setMode(m) {
      state.mode = m;
      state.power = true;
      sendControl();
    }

    function hexToRgb(hex) {
      let result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
      return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
      } : { r: 0, g: 0, b: 0 };
    }

    function rgbToHex(r, g, b) {
      return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
    }

    // Initial sync
    fetchStatus();
    setInterval(fetchStatus, 3000);
  </script>
</body>
</html>
)rawliteral";

// Helper: Wheel color generator for Rainbow effect
uint32_t wheelColor(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Update physical LED strip based on mode
void applyLedState() {
  strip.clear();

  if (!powerOn || currentMode == "off") {
    strip.show();
    return;
  }

  strip.setBrightness(currentBrightness);

  if (currentMode == "solid") {
    for (uint16_t i = 0; i < activeLedCount && i < MAX_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(currentRed, currentGreen, currentBlue));
    }
    strip.show();
  }
  else if (currentMode == "rainbow") {
    for (uint16_t i = 0; i < activeLedCount && i < MAX_LEDS; i++) {
      strip.setPixelColor(i, wheelColor(((i * 256 / activeLedCount) + animStep) & 255));
    }
    strip.show();
  }
  else if (currentMode == "breathing") {
    uint8_t scaledR = (currentRed * breatheVal) / 255;
    uint8_t scaledG = (currentGreen * breatheVal) / 255;
    uint8_t scaledB = (currentBlue * breatheVal) / 255;

    for (uint16_t i = 0; i < activeLedCount && i < MAX_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(scaledR, scaledG, scaledB));
    }
    strip.show();
  }
  else if (currentMode == "wipe") {
    uint16_t fillCount = animStep % (activeLedCount + 1);
    for (uint16_t i = 0; i < activeLedCount && i < MAX_LEDS; i++) {
      if (i < fillCount) {
        strip.setPixelColor(i, strip.Color(currentRed, currentGreen, currentBlue));
      } else {
        strip.setPixelColor(i, 0);
      }
    }
    strip.show();
  }
  else if (currentMode == "chase") {
    uint16_t head = animStep % activeLedCount;
    for (uint16_t i = 0; i < activeLedCount && i < MAX_LEDS; i++) {
      if (i == head || i == (head + 1) % activeLedCount || i == (head + 2) % activeLedCount) {
        strip.setPixelColor(i, strip.Color(currentRed, currentGreen, currentBlue));
      } else {
        strip.setPixelColor(i, strip.Color(currentRed / 8, currentGreen / 8, currentBlue / 8));
      }
    }
    strip.show();
  }
  else if (currentMode == "twinkle") {
    for (uint16_t i = 0; i < activeLedCount && i < MAX_LEDS; i++) {
      if (random(10) > 6) {
        strip.setPixelColor(i, strip.Color(currentRed, currentGreen, currentBlue));
      } else {
        strip.setPixelColor(i, strip.Color(currentRed / 4, currentGreen / 4, currentBlue / 4));
      }
    }
    strip.show();
  }
}

// Non-blocking animation loop tick
void updateAnimation() {
  if (millis() - lastAnimUpdate < animSpeed) return;
  lastAnimUpdate = millis();

  animStep++;

  if (currentMode == "breathing") {
    breatheVal += breatheDirection * 4;
    if (breatheVal >= 250) {
      breatheVal = 250;
      breatheDirection = -1;
    } else if (breatheVal <= 15) {
      breatheVal = 15;
      breatheDirection = 1;
    }
  }

  if (currentMode != "solid" && powerOn) {
    applyLedState();
  }
}

// HTTP Handler: Root Page
void handleRoot() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", HTML_PAGE);
}

// Helper: Build JSON status string
String buildStatusJson() {
  String json = "{";
  json += "\"power\":" + String(powerOn ? "true" : "false") + ",";
  json += "\"red\":" + String(currentRed) + ",";
  json += "\"green\":" + String(currentGreen) + ",";
  json += "\"blue\":" + String(currentBlue) + ",";
  json += "\"brightness\":" + String(currentBrightness) + ",";
  json += "\"count\":" + String(activeLedCount) + ",";
  json += "\"maxLeds\":" + String(MAX_LEDS) + ",";
  json += "\"mode\":\"" + currentMode + "\",";
  json += "\"speed\":" + String(animSpeed) + ",";
  json += "\"ip\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) + "\",";
  json += "\"uptimeSec\":" + String(millis() / 1000);
  json += "}";
  return json;
}

// HTTP Handler: GET /api/status
void handleStatusApi() {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", buildStatusJson());
}

// HTTP Handler: POST/GET /api/control
void handleControlApi() {
  if (server.hasArg("power")) {
    powerOn = (server.arg("power") == "1" || server.arg("power") == "true");
  }
  if (server.hasArg("r")) {
    currentRed = constrain(server.arg("r").toInt(), 0, 255);
  }
  if (server.hasArg("g")) {
    currentGreen = constrain(server.arg("g").toInt(), 0, 255);
  }
  if (server.hasArg("b")) {
    currentBlue = constrain(server.arg("b").toInt(), 0, 255);
  }
  if (server.hasArg("brightness")) {
    currentBrightness = constrain(server.arg("brightness").toInt(), 0, 255);
  }
  if (server.hasArg("count")) {
    activeLedCount = constrain(server.arg("count").toInt(), 1, MAX_LEDS);
  }
  if (server.hasArg("mode")) {
    currentMode = server.arg("mode");
  }
  if (server.hasArg("speed")) {
    animSpeed = constrain(server.arg("speed").toInt(), 5, 1000);
  }

  applyLedState();

  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", buildStatusJson());
}

bool startMdns() {
  if (!MDNS.begin("esp32")) {
    Serial.println("mDNS setup failed. Use the printed IP address instead.");
    return false;
  }

  MDNS.addService("http", "tcp", 80);
  Serial.println("mDNS Active: http://esp32.local (port 80)");
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Initialize NeoPixels
  strip.begin();
  strip.setBrightness(currentBrightness);
  applyLedState();

  Serial.println("\n===========================================");
  Serial.println("ESP32 Addressable LED Strip Web Controller");
  Serial.println("===========================================");

  // Match the working ESP32 projects: use station mode for home Wi-Fi and
  // start the access point only when the station connection fails.
  const bool credentialsConfigured = String(ssid) != "YOUR_WIFI_NAME";
  if (credentialsConfigured) {
    Serial.println("\nConnecting to Wi-Fi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi Connected successfully!");
    Serial.print("Local IP Address: http://");
    Serial.println(WiFi.localIP());
    startMdns();
  } else {
    Serial.println("\nHome Wi-Fi connection timed out. Starting Access Point fallback...");
    WiFi.mode(WIFI_AP);
    IPAddress local_ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(ap_ssid, ap_password);

    Serial.println("SSID:     ESP32-LED-Strip");
    Serial.println("Password: password123");
    Serial.println("URL:      http://192.168.4.1");
    startMdns();
  }

  // Setup WebServer Routes
  server.on("/", handleRoot);
  server.on("/api/status", HTTP_GET, handleStatusApi);
  server.on("/api/control", HTTP_GET, handleControlApi);
  server.on("/api/control", HTTP_POST, handleControlApi);
  server.onNotFound(handleRoot);

  server.begin();
  Serial.println("HTTP Web Server running!");
}

void loop() {
  server.handleClient();
  updateAnimation();
}
