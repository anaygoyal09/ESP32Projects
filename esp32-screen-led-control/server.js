const http = require('http');
const fs = require('fs');
const path = require('path');

const PORT = 3000;
const ESP32_HOST = 'esp32.local'; // ESP32 mDNS hostname on local network

// Local Simulated State (used when ESP32 hardware is offline or testing locally)
let localState = {
  ledState: false,
  brightness: 255,
  rssi: -45,
  ip: "127.0.0.1 (Localhost Mock)",
  uptimeSec: 0
};

const startTime = Date.now();

let esp32Online = false;
let lastCheckTime = 0;

// Helper to forward requests to physical ESP32
function forwardToEsp32(apiPath, callback) {
  const now = Date.now();
  // If we recently failed to connect to ESP32 (within 5 seconds), skip network attempt for fast local response
  if (!esp32Online && (now - lastCheckTime < 5000)) {
    return callback(new Error('ESP32 Offline'), null);
  }

  let called = false;
  const done = (err, data) => {
    if (called) return;
    called = true;
    if (err) {
      esp32Online = false;
      lastCheckTime = Date.now();
    } else {
      esp32Online = true;
    }
    callback(err, data);
  };

  const req = http.get(`http://${ESP32_HOST}${apiPath}`, (res) => {
    let data = '';
    res.on('data', chunk => data += chunk);
    res.on('end', () => {
      try {
        const json = JSON.parse(data);
        done(null, json);
      } catch (err) {
        done(err, null);
      }
    });
  });

  req.on('error', (err) => {
    done(err, null);
  });

  req.setTimeout(800, () => {
    req.destroy();
    done(new Error('ESP32 Connection Timeout'), null);
  });
}

// Embedded Web HTML Page for Localhost
const HTML_CONTENT = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Screen LED Localhost Server</title>
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
      border: 1px solid var(--border-color);
      border-radius: 28px;
      padding: 40px 32px;
      max-width: 440px;
      width: 100%;
      box-shadow: 0 25px 60px -15px rgba(0, 0, 0, 0.7);
      text-align: center;
    }

    .badge-host {
      background: rgba(59, 130, 246, 0.2);
      color: #60a5fa;
      border: 1px solid rgba(59, 130, 246, 0.4);
      padding: 4px 12px;
      border-radius: 12px;
      font-size: 0.75rem;
      font-weight: 700;
      letter-spacing: 0.5px;
      margin-bottom: 12px;
      display: inline-block;
    }

    h1 {
      font-size: 1.6rem;
      margin-bottom: 6px;
      font-weight: 700;
    }

    p.subtitle {
      color: var(--text-muted);
      font-size: 0.88rem;
      margin-bottom: 32px;
    }

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
      transition: all 0.3s ease;
    }

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

    .btn:hover { background: rgba(255, 255, 255, 0.12); transform: translateY(-2px); }
    .btn:active { transform: translateY(0); }

    .btn-on { background: rgba(34, 197, 94, 0.15); color: #4ade80; border-color: rgba(34, 197, 94, 0.3); }
    .btn-on:hover { background: rgba(34, 197, 94, 0.28); }

    .btn-off { background: rgba(239, 68, 68, 0.15); color: #f87171; border-color: rgba(239, 68, 68, 0.3); }
    .btn-off:hover { background: rgba(239, 68, 68, 0.28); }

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

    .stat-val { font-size: 1rem; font-weight: 700; color: var(--text-main); }
    .stat-lbl { font-size: 0.72rem; color: var(--text-muted); text-transform: uppercase; margin-top: 4px; }
  </style>
</head>
<body>

  <div class="container">
    <div class="badge-host">SERVER RUNNING ON LOCALHOST:3000</div>
    <h1>ESP32 Screen Control</h1>
    <p class="subtitle">Click light bulb on screen to make LED turn on</p>

    <div id="bulb" class="bulb-wrapper" onclick="toggleLed()">
      <svg class="bulb-svg" viewBox="0 0 24 24">
        <path d="M9 18h6m-4 3h2M12 2a7 7 0 0 0-7 7c0 2.38 1.19 4.47 3 5.74V17a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1v-2.26c1.81-1.27 3-3.36 3-5.74a7 7 0 0 0-7-7z"/>
      </svg>
    </div>

    <div id="statusBadge" class="status-badge">LED OFF</div>

    <div class="btn-grid">
      <button class="btn btn-on" onclick="setLedState(true)">Turn ON</button>
      <button class="btn btn-off" onclick="setLedState(false)">Turn OFF</button>
      <button class="btn" onclick="toggleLed()">Toggle</button>
      <button class="btn" onclick="triggerBlink()">Pulse / Blink</button>
    </div>

    <div class="slider-section">
      <div class="slider-header">
        <span>LED Brightness</span>
        <span id="sliderVal">100%</span>
      </div>
      <input type="range" id="brightnessSlider" min="0" max="255" value="255" oninput="onBrightnessInput(this.value)">
    </div>

    <div class="stats-grid">
      <div class="stat-card">
        <div id="targetIp" class="stat-val">http://localhost:3000</div>
        <div class="stat-lbl">Local Host URL</div>
      </div>
      <div class="stat-card">
        <div id="uptimeVal" class="stat-val">0s</div>
        <div class="stat-lbl">Server Uptime</div>
      </div>
    </div>
  </div>

  <script>
    function updateUI(data) {
      const bulb = document.getElementById('bulb');
      const statusBadge = document.getElementById('statusBadge');
      const sliderVal = document.getElementById('sliderVal');
      const brightnessSlider = document.getElementById('brightnessSlider');
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
      uptimeVal.innerText = data.uptimeSec + 's';
    }

    function fetchStatus() {
      fetch('/api/status')
        .then(res => res.json())
        .then(data => updateUI(data))
        .catch(err => console.error(err));
    }

    function toggleLed() {
      fetch('/api/led/toggle')
        .then(res => res.json())
        .then(data => updateUI(data));
    }

    function setLedState(on) {
      fetch(on ? '/api/led/on' : '/api/led/off')
        .then(res => res.json())
        .then(data => updateUI(data));
    }

    function onBrightnessInput(val) {
      document.getElementById('sliderVal').innerText = Math.round((val / 255) * 100) + '%';
      fetch('/api/led/set?brightness=' + val)
        .then(res => res.json())
        .then(data => updateUI(data));
    }

    function triggerBlink() {
      fetch('/api/led/blink')
        .then(res => res.json())
        .then(data => updateUI(data));
    }

    setInterval(fetchStatus, 400);
    fetchStatus();
  </script>
</body>
</html>`;

const server = http.createServer((req, res) => {
  const urlParts = req.url.split('?');
  const pathname = urlParts[0];

  res.setHeader('Access-Control-Allow-Origin', '*');

  if (pathname === '/') {
    res.writeHead(200, { 'Content-Type': 'text/html' });
    res.end(HTML_CONTENT);
    return;
  }

  // Handle LED API Requests
  if (pathname.startsWith('/api/led/') || pathname === '/api/status') {

    // First attempt to forward request to real ESP32 if online
    forwardToEsp32(req.url, (err, espData) => {
      if (!err && espData) {
        // ESP32 responded! Return real ESP32 status
        localState = espData;
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(espData));
        return;
      }

      // If ESP32 is offline, operate in Local Simulated mode
      if (pathname === '/api/led/on') {
        localState.ledState = true;
      } else if (pathname === '/api/led/off') {
        localState.ledState = false;
      } else if (pathname === '/api/led/toggle') {
        localState.ledState = !localState.ledState;
      } else if (pathname === '/api/led/set') {
        const queryParams = new URLSearchParams(urlParts[1] || '');
        const br = parseInt(queryParams.get('brightness') || '255');
        localState.brightness = br;
        localState.ledState = br > 0;
      } else if (pathname === '/api/led/blink') {
        localState.ledState = true;
      }

      localState.uptimeSec = Math.floor((Date.now() - startTime) / 1000);

      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify(localState));
    });
    return;
  }

  res.writeHead(404, { 'Content-Type': 'text/plain' });
  res.end('404 Not Found');
});

server.listen(PORT, () => {
  console.log('====================================================');
  console.log(`  Localhost Server Running at: http://localhost:${PORT}`);
  console.log(`  ESP32 Target Address: http://${ESP32_HOST}`);
  console.log('====================================================');
});
