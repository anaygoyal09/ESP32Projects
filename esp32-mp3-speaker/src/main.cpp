#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include "Audio.h"

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

// Fallback Access Point Credentials if Wi-Fi fails
const char* ap_ssid     = "ESP32-Audio-Network";
const char* ap_password = "password123";

// Audio is initialized only when playback is requested. This lets Wi-Fi and the
// web server boot independently and keeps GPIO25 quiet during startup.
Audio* audio = nullptr;
WebServer server(80);

bool isPlaying = false;
uint8_t currentVolume = 15; // 0 to 21
String currentUrl = "/audio.mp3"; // Default to your local MP3 file in LittleFS

const uint8_t BUTTON_PIN = 32;
const unsigned long BUTTON_DEBOUNCE_MS = 50;
bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;
unsigned long lastButtonChange = 0;

// Start mDNS after either the station interface or fallback access point is up.
// This keeps http://esp32.local available in both connection modes.
void startMdns() {
  if (MDNS.begin("esp32")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[mDNS] Active! Website URL: http://esp32.local");
  } else {
    Serial.println("[mDNS] Failed to start. Use the IP address shown above instead.");
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
  <title>ESP32 Speaker & MP3 Controller</title>
  <style>
    :root {
      --bg-color: #0b0f19;
      --card-bg: rgba(30, 41, 59, 0.75);
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --glow-on: #00f2fe;
      --glow-on-bg: rgba(0, 242, 254, 0.15);
      --danger: #ef4444;
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
      background: linear-gradient(135deg, #00f2fe 0%, #4facfe 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }

    p.subtitle {
      color: var(--text-muted);
      font-size: 0.88rem;
      margin-bottom: 28px;
    }

    /* Speaker Graphic Switch */
    .speaker-wrapper {
      position: relative;
      width: 160px;
      height: 160px;
      margin: 0 auto 28px;
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

    .speaker-wrapper:hover {
      transform: scale(1.05);
      border-color: rgba(0, 242, 254, 0.3);
    }

    .speaker-wrapper:active {
      transform: scale(0.95);
    }

    .speaker-svg {
      width: 75px;
      height: 75px;
      fill: none;
      stroke: #64748b;
      stroke-width: 1.8;
      stroke-linecap: round;
      stroke-linejoin: round;
      transition: all 0.3s ease;
    }

    .speaker-wrapper.playing {
      border-color: var(--glow-on);
      background: var(--glow-on-bg);
      box-shadow: 0 0 50px rgba(0, 242, 254, 0.4), inset 0 0 25px rgba(0, 242, 254, 0.2);
    }

    .speaker-wrapper.playing .speaker-svg {
      stroke: #00f2fe;
      filter: drop-shadow(0 0 10px #00f2fe);
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

    .status-badge.playing {
      background: rgba(0, 242, 254, 0.18);
      color: #00f2fe;
      border-color: rgba(0, 242, 254, 0.4);
    }

    /* Control Buttons */
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

    .btn-play {
      background: rgba(0, 242, 254, 0.18);
      color: #00f2fe;
      border-color: rgba(0, 242, 254, 0.35);
    }
    .btn-play:hover { background: rgba(0, 242, 254, 0.3); }

    .btn-stop {
      background: rgba(239, 68, 68, 0.15);
      color: #f87171;
      border-color: rgba(239, 68, 68, 0.3);
    }
    .btn-stop:hover { background: rgba(239, 68, 68, 0.28); }

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

    /* URL Box */
    .url-section {
      background: rgba(15, 23, 42, 0.6);
      border: 1px solid var(--border-color);
      border-radius: 16px;
      padding: 16px;
      text-align: left;
    }

    .url-section label {
      display: block;
      font-size: 0.75rem;
      color: var(--text-muted);
      margin-bottom: 8px;
      font-weight: 600;
      text-transform: uppercase;
    }

    .url-input {
      width: 100%;
      background: rgba(0, 0, 0, 0.4);
      border: 1px solid var(--border-color);
      border-radius: 10px;
      padding: 10px;
      color: var(--text-main);
      font-size: 0.85rem;
      outline: none;
      box-sizing: border-color 0.2s ease;
      margin-bottom: 10px;
    }
  </style>
</head>
<body>

  <div class="container">
    <h1>ESP32 Audio Player</h1>
    <p class="subtitle">Click speaker to start/stop MP3 playback on P25</p>

    <!-- Interactive Speaker Icon -->
    <div id="speakerBtn" class="speaker-wrapper" onclick="toggleAudio()">
      <svg class="speaker-svg" viewBox="0 0 24 24">
        <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
        <path d="M19.07 4.93a10 10 0 0 1 0 14.14M15.54 8.46a5 5 0 0 1 0 7.07"/>
      </svg>
    </div>

    <div id="statusBadge" class="status-badge">AUDIO STOPPED</div>

    <!-- Controls -->
    <div class="btn-grid">
      <button class="btn btn-play" onclick="playAudio()">Start MP3</button>
      <button class="btn btn-stop" onclick="stopAudio()">Stop MP3</button>
    </div>

    <!-- Volume Control -->
    <div class="slider-section">
      <div class="slider-header">
        <span>Volume</span>
        <span id="volVal">70%</span>
      </div>
      <input type="range" id="volSlider" min="0" max="21" value="15" oninput="setVolume(this.value)">
    </div>

    <!-- Custom MP3 File / URL -->
    <div class="url-section">
      <label>MP3 File / Web Stream</label>
      <input type="text" id="audioUrl" class="url-input" value="/audio.mp3">
      <button class="btn" style="width:100%" onclick="playCustomUrl()">Load & Play</button>
    </div>
  </div>

  <script>
    function updateUI(data) {
      const spk = document.getElementById('speakerBtn');
      const badge = document.getElementById('statusBadge');
      const volVal = document.getElementById('volVal');
      const volSlider = document.getElementById('volSlider');

      if (data.playing) {
        spk.classList.add('playing');
        badge.classList.add('playing');
        badge.innerText = 'AUDIO PLAYING';
      } else {
        spk.classList.remove('playing');
        badge.classList.remove('playing');
        badge.innerText = 'AUDIO STOPPED';
      }

      const pct = Math.round((data.volume / 21) * 100);
      volVal.innerText = pct + '%';
      if (document.activeElement !== volSlider) {
        volSlider.value = data.volume;
      }
    }

    function fetchStatus() {
      fetch('/api/status')
        .then(res => res.json())
        .then(data => updateUI(data))
        .catch(err => console.error(err));
    }

    function playAudio() {
      const url = document.getElementById('audioUrl').value;
      fetch('/api/play?url=' + encodeURIComponent(url))
        .then(res => res.json())
        .then(data => updateUI(data));
    }

    function stopAudio() {
      fetch('/api/stop')
        .then(res => res.json())
        .then(data => updateUI(data));
    }

    function toggleAudio() {
      fetch('/api/toggle')
        .then(res => res.json())
        .then(data => updateUI(data));
    }

    function setVolume(val) {
      document.getElementById('volVal').innerText = Math.round((val / 21) * 100) + '%';
      fetch('/api/volume?level=' + val)
        .then(res => res.json())
        .then(data => updateUI(data));
    }

    function playCustomUrl() {
      playAudio();
    }

    setInterval(fetchStatus, 1000);
    fetchStatus();
  </script>
</body>
</html>
)rawliteral";

// -------------------------------------------------------------
// WebServer HTTP Route Handlers
// -------------------------------------------------------------

void sendStatusJson() {
  bool playing = (audio != nullptr && audio->isRunning());
  String json = "{";
  json += "\"playing\":" + String(playing ? "true" : "false") + ",";
  json += "\"volume\":" + String(currentVolume) + ",";
  json += "\"url\":\"" + currentUrl + "\"";
  json += "}";

  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", HTML_PAGE);
}

void handleStatus() {
  sendStatusJson();
}

bool initializeAudio() {
  if (audio != nullptr) return true;

  Serial.println("[Audio] Initializing GPIO25 internal DAC...");
  audio = new Audio(true, I2S_DAC_CHANNEL_RIGHT_EN);
  if (audio == nullptr) {
    Serial.println("[Audio] Initialization failed: out of memory");
    return false;
  }

  Serial.println("[Audio] Audio subsystem initialized cleanly!");
  return true;
}

void startPlayback(String url) {
  if (!initializeAudio()) return;
  currentUrl = url;
  if (url.startsWith("/")) {
    if (LittleFS.exists(url)) {
      Serial.printf("Playing local file from LittleFS: %s\n", url.c_str());
      isPlaying = audio->connecttoFS(LittleFS, url.c_str());
    } else {
      Serial.printf("File %s not found in LittleFS!\n", url.c_str());
      isPlaying = false;
    }
  } else {
    Serial.printf("Playing web stream: %s\n", url.c_str());
    isPlaying = audio->connecttohost(url.c_str());
  }
  if (isPlaying) audio->setVolume(currentVolume);
}

void handlePlay() {
  if (server.hasArg("url") && server.arg("url").length() > 0) {
    currentUrl = server.arg("url");
  }
  startPlayback(currentUrl);
  sendStatusJson();
}

void handleStop() {
  Serial.println("Stopping Audio Playback...");
  if (audio != nullptr) {
    audio->stopSong();
  }
  isPlaying = false;
  sendStatusJson();
}

void togglePlayback() {
  if (audio != nullptr && audio->isRunning()) {
    audio->stopSong();
    isPlaying = false;
  } else {
    startPlayback(currentUrl);
  }
}

void handleToggle() {
  togglePlayback();
  sendStatusJson();
}

void updatePhysicalButton() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastButtonChange = millis();
    lastButtonReading = reading;
  }

  if ((millis() - lastButtonChange) >= BUTTON_DEBOUNCE_MS &&
      reading != stableButtonState) {
    stableButtonState = reading;
    if (stableButtonState == LOW) {
      Serial.println("[Button] Pressed - toggling audio playback");
      togglePlayback();
    }
  }
}

void handleVolume() {
  if (server.hasArg("level")) {
    int val = server.arg("level").toInt();
    if (val < 0) val = 0;
    if (val > 21) val = 21;
    currentVolume = (uint8_t)val;
    if (audio != nullptr) {
      audio->setVolume(currentVolume);
    }
  }
  sendStatusJson();
}

void handleNotFound() {
  server.sendHeader("Connection", "close");
  server.send(404, "text/plain", "404 Not Found");
}

// -------------------------------------------------------------
// Setup & Main Loop
// -------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);

  // The button connects GPIO32 to GND when pressed; no external resistor needed.
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("\n===========================================");
  Serial.println("=== ESP32 MP3 Web Controller Starting ===");
  Serial.println("===========================================");

  // Mount LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("[FS] LittleFS Mount Failed!");
  } else {
    Serial.println("[FS] LittleFS Mounted Successfully!");
  }

  // Connect to Wi-Fi
  Serial.print("[WiFi] Connecting to: ");
  Serial.println(ssid);
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("esp32");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n===========================================");
    Serial.println("[WiFi] Connected to Home Network!");
    Serial.print("[WiFi] Local IP Address: http://");
    Serial.println(WiFi.localIP());

    Serial.println("===========================================");
  } else {
    Serial.println("\n[WiFi] Home Wi-Fi timed out. Starting Access Point fallback...");
    WiFi.mode(WIFI_AP);
    IPAddress local_ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(ap_ssid, ap_password);

    Serial.println("===========================================");
    Serial.println("[AP] Access Point Created!");
    Serial.println("SSID:     ESP32-Audio-Network");
    Serial.println("Password: password123");
    Serial.println("URL:      http://192.168.4.1");
    Serial.println("===========================================");
  }

  startMdns();

  // Set WebServer Endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/play", HTTP_GET, handlePlay);
  server.on("/api/stop", HTTP_GET, handleStop);
  server.on("/api/toggle", HTTP_GET, handleToggle);
  server.on("/api/volume", HTTP_GET, handleVolume);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("[HTTP] Web Server running!");

  // Hold the transistor off until audio playback is explicitly requested.
  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);
  Serial.println("[Audio] GPIO25 muted; audio will initialize when Play is pressed.");
}

void loop() {
  server.handleClient();
  updatePhysicalButton();
  if (audio != nullptr) {
    audio->loop();
  }
}
