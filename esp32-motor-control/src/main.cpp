#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// -------------------------------------------------------------
// Read Wi-Fi credentials from .env via load_env.py
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
const char* ap_ssid     = "ESP32-Motor-Network";
const char* ap_password = "password123";

// -------------------------------------------------------------
// Hardware Pin Definitions
// -------------------------------------------------------------
// L298N / L293D DC Motor H-Bridge Pins
constexpr uint8_t MOTOR_ENA_PIN = 25; // PWM Speed Control
constexpr uint8_t MOTOR_IN1_PIN = 26; // Direction Pin 1
constexpr uint8_t MOTOR_IN2_PIN = 27; // Direction Pin 2

// Servo Motor Pin
constexpr uint8_t SERVO_PIN     = 19; // Servo PWM Output

// PWM Settings
constexpr uint8_t  DC_PWM_CHANNEL    = 0;
constexpr uint32_t DC_PWM_FREQ       = 5000; // 5 kHz
constexpr uint8_t  DC_PWM_BITS       = 8;    // 0-255

constexpr uint8_t  SERVO_PWM_CHANNEL = 1;
constexpr uint32_t SERVO_PWM_FREQ    = 50;   // 50 Hz standard servo frequency
constexpr uint8_t  SERVO_PWM_BITS    = 16;   // 16-bit resolution for precise pulse control

// -------------------------------------------------------------
// Global Motor State Variables
// -------------------------------------------------------------
enum MotorDirection {
  MOTOR_STOP = 0,
  MOTOR_FORWARD = 1,
  MOTOR_REVERSE = 2
};

MotorDirection currentDirection = MOTOR_STOP;
uint8_t currentSpeedPercent = 0; // 0% to 100%
uint8_t currentSpeedPWM = 0;     // 0 to 255
uint8_t currentServoAngle = 90;  // 0° to 180°

WebServer server(80);

// Helper function to convert angle (0-180) to 16-bit duty cycle at 50Hz
uint32_t angleToServoDuty(uint8_t angle) {
  // Standard servo pulses range from ~500us (0 deg) to ~2500us (180 deg)
  // At 50Hz, period = 20,000us. 16-bit max = 65535.
  // Duty (0 deg)   = (500 / 20000) * 65535 = ~1638
  // Duty (180 deg) = (2500 / 20000) * 65535 = ~8191
  uint32_t pulseUs = map(angle, 0, 180, 500, 2500);
  return (pulseUs * 65535) / 20000;
}

void applyMotorState() {
  currentSpeedPWM = map(currentSpeedPercent, 0, 100, 0, 255);

  // Disable the bridge before changing direction. This prevents a brief
  // full-power pulse and avoids hard reversing the motor under load.
  ledcWrite(DC_PWM_CHANNEL, 0);

  switch (currentDirection) {
    case MOTOR_FORWARD:
      digitalWrite(MOTOR_IN1_PIN, HIGH);
      digitalWrite(MOTOR_IN2_PIN, LOW);
      ledcWrite(DC_PWM_CHANNEL, currentSpeedPWM);
      break;

    case MOTOR_REVERSE:
      digitalWrite(MOTOR_IN1_PIN, LOW);
      digitalWrite(MOTOR_IN2_PIN, HIGH);
      ledcWrite(DC_PWM_CHANNEL, currentSpeedPWM);
      break;

    case MOTOR_STOP:
    default:
      digitalWrite(MOTOR_IN1_PIN, LOW);
      digitalWrite(MOTOR_IN2_PIN, LOW);
      ledcWrite(DC_PWM_CHANNEL, 0);
      break;
  }
}

void applyServoState() {
  uint32_t duty = angleToServoDuty(currentServoAngle);
  ledcWrite(SERVO_PWM_CHANNEL, duty);
}

// -------------------------------------------------------------
// HTML Dashboard Template
// -------------------------------------------------------------
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Motor Control Hub</title>
  <style>
    :root {
      --bg-color: #0b0f19;
      --card-bg: #151c2c;
      --card-border: #232d42;
      --text-main: #f1f5f9;
      --text-muted: #94a3b8;
      --accent-blue: #3b82f6;
      --accent-green: #10b981;
      --accent-red: #ef4444;
      --accent-amber: #f59e0b;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; }

    body {
      background: var(--bg-color);
      color: var(--text-main);
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      padding: 20px;
    }

    .container {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: 24px;
      padding: 32px;
      max-width: 480px;
      width: 100%;
      box-shadow: 0 20px 40px rgba(0,0,0,0.6);
    }

    .header {
      text-align: center;
      margin-bottom: 24px;
    }

    .header h1 {
      font-size: 1.75rem;
      font-weight: 700;
      letter-spacing: -0.5px;
      color: var(--text-main);
    }

    .header p {
      color: var(--text-muted);
      font-size: 0.875rem;
      margin-top: 4px;
    }

    .connection {
      display: inline-flex;
      align-items: center;
      gap: 7px;
      margin-top: 10px;
      color: var(--text-muted);
      font-size: 0.78rem;
    }

    .connection-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: var(--accent-amber);
      box-shadow: 0 0 8px rgba(245, 158, 11, 0.5);
    }

    .connection.online .connection-dot {
      background: var(--accent-green);
      box-shadow: 0 0 8px rgba(16, 185, 129, 0.65);
    }

    .connection.offline .connection-dot {
      background: var(--accent-red);
      box-shadow: 0 0 8px rgba(239, 68, 68, 0.65);
    }

    .section {
      background: rgba(255, 255, 255, 0.02);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 20px;
      margin-bottom: 20px;
    }

    .section-title {
      font-size: 0.9rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 1px;
      color: var(--accent-blue);
      margin-bottom: 16px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    .btn-group {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr;
      gap: 10px;
      margin-bottom: 16px;
    }

    button.btn {
      background: #1e293b;
      border: 1px solid var(--card-border);
      color: var(--text-main);
      padding: 12px;
      border-radius: 12px;
      font-weight: 600;
      font-size: 0.85rem;
      cursor: pointer;
      transition: all 0.2s ease;
    }

    button.btn:hover {
      background: #334155;
    }

    button.btn:focus-visible, input[type=range]:focus-visible {
      outline: 3px solid rgba(59, 130, 246, 0.65);
      outline-offset: 3px;
    }

    button.btn.active-fwd {
      background: var(--accent-green);
      border-color: var(--accent-green);
      box-shadow: 0 0 15px rgba(16, 185, 129, 0.4);
    }

    button.btn.active-rev {
      background: var(--accent-amber);
      border-color: var(--accent-amber);
      box-shadow: 0 0 15px rgba(245, 158, 11, 0.4);
    }

    button.btn.active-stop {
      background: var(--accent-red);
      border-color: var(--accent-red);
      box-shadow: 0 0 15px rgba(239, 68, 68, 0.4);
    }

    .slider-container {
      margin-top: 12px;
    }

    .slider-label {
      display: flex;
      justify-content: space-between;
      font-size: 0.85rem;
      color: var(--text-muted);
      margin-bottom: 8px;
    }

    input[type=range] {
      width: 100%;
      height: 8px;
      border-radius: 4px;
      background: #1e293b;
      outline: none;
      -webkit-appearance: none;
    }

    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 22px;
      height: 22px;
      border-radius: 50%;
      background: var(--accent-blue);
      cursor: pointer;
      box-shadow: 0 0 10px rgba(59, 130, 246, 0.6);
    }

    .stats-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
      margin-top: 12px;
    }

    .stat-card {
      background: rgba(15, 23, 42, 0.5);
      border: 1px solid var(--card-border);
      border-radius: 12px;
      padding: 12px;
      text-align: center;
    }

    .stat-val {
      font-size: 1.2rem;
      font-weight: 700;
      color: var(--text-main);
    }

    .stat-lbl {
      font-size: 0.75rem;
      color: var(--text-muted);
      margin-top: 2px;
    }

    .footer {
      color: var(--text-muted);
      font-size: 0.72rem;
      text-align: center;
      margin-top: 18px;
    }

    @media (max-width: 420px) {
      body { padding: 12px; }
      .container { padding: 22px 16px; border-radius: 18px; }
      .header h1 { font-size: 1.45rem; }
      button.btn { padding: 11px 7px; font-size: 0.76rem; }
    }

    @media (prefers-reduced-motion: reduce) {
      * { transition: none !important; }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>⚙️ ESP32 Motor Hub</h1>
      <p>DC H-Bridge Driver & Servo Positioner</p>
      <div id="connection" class="connection">
        <span class="connection-dot"></span>
        <span id="connection-text">Connecting...</span>
      </div>
    </div>

    <!-- DC Motor Control -->
    <div class="section">
      <div class="section-title">
        <span>DC Motor Power</span>
        <span id="dc-status-badge" style="color: var(--text-muted); font-weight: 700;">OFF</span>
      </div>

      <!-- Main Power ON / OFF Switch -->
      <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 16px;">
        <button id="btn-power-on" class="btn" style="padding: 16px; font-size: 1.1rem; background: #059669; border-color: #10b981;" onclick="turnOnMotor()">⚡ TURN ON</button>
        <button id="btn-power-off" class="btn active-stop" style="padding: 16px; font-size: 1.1rem;" onclick="turnOffMotor()">🛑 TURN OFF</button>
      </div>

      <div class="btn-group">
        <button id="btn-fwd" class="btn" onclick="setDirection('FORWARD')">FORWARD</button>
        <button id="btn-stop" class="btn active-stop" onclick="setDirection('STOP')">STOP</button>
        <button id="btn-rev" class="btn" onclick="setDirection('REVERSE')">REVERSE</button>
      </div>

      <div class="slider-container">
        <div class="slider-label">
          <span>Speed Control (PWM)</span>
          <span id="speed-val">0%</span>
        </div>
        <input type="range" id="speed-slider" min="0" max="100" value="0" aria-label="DC motor speed" oninput="previewSpeed(this.value)" onchange="updateSpeed(this.value)">
      </div>
    </div>

    <!-- Servo Motor Control -->
    <div class="section">
      <div class="section-title">
        <span>Servo Motor</span>
        <span id="servo-val" style="color: var(--accent-blue);">90°</span>
      </div>

      <div class="slider-container">
        <div class="slider-label">
          <span>Angle (0° - 180°)</span>
        </div>
        <input type="range" id="servo-slider" min="0" max="180" value="90" aria-label="Servo angle" oninput="previewServo(this.value)" onchange="updateServo(this.value)">
      </div>
    </div>

    <!-- System Telemetry -->
    <div class="stats-grid">
      <div class="stat-card">
        <div id="pwm-val" class="stat-val">0 / 255</div>
        <div class="stat-lbl">PWM Duty Cycle</div>
      </div>
      <div class="stat-card">
        <div id="uptime-val" class="stat-val">0s</div>
        <div class="stat-lbl">System Uptime</div>
      </div>
    </div>
    <p class="footer">Emergency stop: press the red TURN OFF button</p>
  </div>

  <script>
    let currentDir = 'STOP';
    let speedTimer;
    let servoTimer;

    function setConnection(isOnline) {
      const connection = document.getElementById('connection');
      connection.className = 'connection ' + (isOnline ? 'online' : 'offline');
      document.getElementById('connection-text').innerText = isOnline ? 'Controller online' : 'Controller unreachable';
    }

    function request(url) {
      return fetch(url, { cache: 'no-store' })
        .then(res => {
          if (!res.ok) throw new Error('HTTP ' + res.status);
          return res.json();
        })
        .then(data => {
          setConnection(true);
          updateUI(data);
          return data;
        })
        .catch(err => {
          setConnection(false);
          console.error('Controller request failed:', err);
        });
    }

    function turnOnMotor() {
      // Start gently instead of jumping to full power.
      const selectedSpeed = Number(document.getElementById('speed-slider').value);
      const startupSpeed = selectedSpeed > 0 ? selectedSpeed : 25;
      request(`/api/motor?dir=FORWARD&speed=${startupSpeed}`);
    }

    function turnOffMotor() {
      // Turn motor OFF (STOP, 0% speed)
      request('/api/motor?dir=STOP&speed=0');
    }

    function setDirection(dir) {
      currentDir = dir;
      const speed = document.getElementById('speed-slider').value;
      request(`/api/motor?dir=${dir}&speed=${speed}`);
    }

    function updateSpeed(speed) {
      clearTimeout(speedTimer);
      request(`/api/motor?dir=${currentDir}&speed=${speed}`);
    }

    function previewSpeed(speed) {
      document.getElementById('speed-val').innerText = speed + '%';
      clearTimeout(speedTimer);
      speedTimer = setTimeout(() => updateSpeed(speed), 120);
    }

    function updateServo(angle) {
      clearTimeout(servoTimer);
      request(`/api/servo?angle=${angle}`);
    }

    function previewServo(angle) {
      document.getElementById('servo-val').innerText = angle + '°';
      clearTimeout(servoTimer);
      servoTimer = setTimeout(() => updateServo(angle), 80);
    }

    function pollStatus() {
      request('/api/status');
    }

    function updateUI(data) {
      currentDir = data.direction;
      const badge = document.getElementById('dc-status-badge');
      if (data.direction === 'STOP' || data.speedPercent === 0) {
        badge.innerText = 'OFF';
        badge.style.color = '#ef4444';
      } else {
        badge.innerText = 'ON (' + data.direction + ')';
        badge.style.color = '#10b981';
      }

      const fwdBtn = document.getElementById('btn-fwd');
      const stopBtn = document.getElementById('btn-stop');
      const revBtn = document.getElementById('btn-rev');

      fwdBtn.className = 'btn' + (data.direction === 'FORWARD' ? ' active-fwd' : '');
      stopBtn.className = 'btn' + (data.direction === 'STOP' ? ' active-stop' : '');
      revBtn.className = 'btn' + (data.direction === 'REVERSE' ? ' active-rev' : '');

      document.getElementById('speed-slider').value = data.speedPercent;
      document.getElementById('speed-val').innerText = data.speedPercent + '%';

      document.getElementById('servo-slider').value = data.servoAngle;
      document.getElementById('servo-val').innerText = data.servoAngle + '°';

      document.getElementById('pwm-val').innerText = data.speedPwm + ' / 255';
      document.getElementById('uptime-val').innerText = data.uptimeSec + 's';
    }

    pollStatus();
    setInterval(pollStatus, 1000);
  </script>
</body>
</html>
)rawliteral";

// -------------------------------------------------------------
// Web Server Route Handlers
// -------------------------------------------------------------
void handleRoot() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", HTML_PAGE);
}

void handleStatusApi() {
  String json = "{";
  json += "\"direction\":\"" + String(currentDirection == MOTOR_FORWARD ? "FORWARD" : (currentDirection == MOTOR_REVERSE ? "REVERSE" : "STOP")) + "\",";
  json += "\"speedPercent\":" + String(currentSpeedPercent) + ",";
  json += "\"speedPwm\":" + String(currentSpeedPWM) + ",";
  json += "\"servoAngle\":" + String(currentServoAngle) + ",";
  json += "\"uptimeSec\":" + String(millis() / 1000);
  json += "}";

  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleMotorApi() {
  if (server.hasArg("dir")) {
    String dirArg = server.arg("dir");
    if (dirArg.equalsIgnoreCase("FORWARD")) {
      currentDirection = MOTOR_FORWARD;
    } else if (dirArg.equalsIgnoreCase("REVERSE")) {
      currentDirection = MOTOR_REVERSE;
    } else {
      currentDirection = MOTOR_STOP;
    }
  }

  if (server.hasArg("speed")) {
    int spd = server.arg("speed").toInt();
    currentSpeedPercent = constrain(spd, 0, 100);
  }

  applyMotorState();
  handleStatusApi();
}

void handleServoApi() {
  if (server.hasArg("angle")) {
    int ang = server.arg("angle").toInt();
    currentServoAngle = constrain(ang, 0, 180);
    applyServoState();
  }
  handleStatusApi();
}

// -------------------------------------------------------------
// Setup & Loop
// -------------------------------------------------------------
void setup() {
  // Put the H-bridge in a safe state before Serial, Wi-Fi, or any delays.
  // ENA must also have its L298N jumper removed so GPIO 25 controls it.
  pinMode(MOTOR_ENA_PIN, OUTPUT);
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);
  digitalWrite(MOTOR_ENA_PIN, LOW);
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);

  Serial.begin(115200);
  delay(100);

  // Configure DC Motor LEDC PWM Channel
  ledcSetup(DC_PWM_CHANNEL, DC_PWM_FREQ, DC_PWM_BITS);
  ledcAttachPin(MOTOR_ENA_PIN, DC_PWM_CHANNEL);
  ledcWrite(DC_PWM_CHANNEL, 0);

  // Configure Servo LEDC PWM Channel
  ledcSetup(SERVO_PWM_CHANNEL, SERVO_PWM_FREQ, SERVO_PWM_BITS);
  ledcAttachPin(SERVO_PIN, SERVO_PWM_CHANNEL);
  applyServoState();

  Serial.println("\nInitializing Wi-Fi Connection...");
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

    if (MDNS.begin("esp32motor")) {
      Serial.println("mDNS Active! Website URL: http://esp32motor.local");
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
    Serial.println("SSID:     ESP32-Motor-Network");
    Serial.println("Password: password123");
    Serial.println("URL:      http://192.168.4.1");
    Serial.println("===========================================");
  }

  server.on("/", handleRoot);
  server.on("/api/status", handleStatusApi);
  server.on("/api/motor", handleMotorApi);
  server.on("/api/servo", handleServoApi);
  server.begin();
  Serial.println("Motor Control Web Server running!");
}

void loop() {
  server.handleClient();
}
