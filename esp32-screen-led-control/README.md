# ESP32 Screen Click LED Control

A high-performance ESP32 web server project that hosts an interactive web dashboard on your local network. Simply click anywhere on the screen (the interactive light bulb graphic, toggle switch, or action buttons) in your web browser to turn the ESP32's LED ON, OFF, or adjust its brightness!

## Features

- **Interactive Screen Click**: Tap or click the central glowing light bulb graphic on screen to toggle the physical LED state instantly.
- **Real-Time Web Dashboard**: Built with modern dark mode aesthetic, glassmorphism, fluid animations, and responsiveness.
- **Brightness Control**: Dynamic slider to adjust LED PWM brightness from 0 to 100%.
- **Special Effects**: Quick action buttons for ON, OFF, Toggle, Pulse wave, and 3x Blink sequence.
- **mDNS Hostname Support**: Access via `http://esp32LED.local` on your local network without typing IP addresses.
- **Fallback Access Point**: Automatically creates Wi-Fi Access Point `ESP32-LED-Control` (`password123`) if home Wi-Fi is unavailable.
- **Secure Credentials**: Uses `.env` configuration loaded at build time via `load_env.py`.

## Hardware Setup

- **ESP32 Development Board** (e.g. ESP32-WROOM-32, NodeMCU ESP32)
- **Onboard LED**: Uses GPIO 2 by default (standard builtin LED on most ESP32 boards).
- *(Optional)* **External LED**: Connect an external LED anode (long leg) to GPIO 2 (or any preferred pin) with a 220Ω resistor to GND.

## Getting Started

1. **Configure Wi-Fi**:
   Edit `.env` in the root folder with your Wi-Fi credentials:
   ```env
   WIFI_SSID="Your_WiFi_Name"
   WIFI_PASSWORD="Your_WiFi_Password"
   ```

2. **Build and Upload**:
   Using PlatformIO CLI or VS Code PlatformIO extension:
   ```bash
   pio run -t upload
   ```

3. **Monitor Serial Output**:
   ```bash
   pio device monitor
   ```

4. **Open Web Dashboard**:
   Once connected, open your browser and navigate to:
   - `http://esp32LED.local`
   - or the IP address printed in the Serial Monitor (e.g., `http://192.168.1.xxx`).

## REST API Endpoints

| Endpoint | Method | Description |
| :--- | :--- | :--- |
| `/` | `GET` | Serves the interactive web dashboard. |
| `/api/status` | `GET` | Returns current state JSON `{ "ledState": true, "brightness": 255, "rssi": -52, "ip": "...", "uptimeSec": 42 }` |
| `/api/led/on` | `GET` | Turns LED ON. |
| `/api/led/off` | `GET` | Turns LED OFF. |
| `/api/led/toggle` | `GET` | Toggles LED state. |
| `/api/led/set?brightness=0..255` | `GET` | Sets specific brightness intensity. |
| `/api/led/blink` | `GET` | Triggers a 3x blink animation. |
