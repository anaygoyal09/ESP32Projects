# ESP32 Mini Projects 🚀

A collection of hands-on ESP32 micro-controller mini-projects built with **PlatformIO**, featuring web dashboards, hardware drivers, and sensor/button controls.

---

## 📂 Projects Overview

### 1. ESP32 MP3 Speaker Driver (`esp32-mp3-speaker`)
A web-controlled audio player that streams MP3 files stored in ESP32 flash memory (LittleFS) to a physical speaker via an S8050 NPN transistor driver.

* **URL:** `http://esp32.local`
* **Key Components:** NodeMCU-32 ESP32, S8050 Transistor, 1kΩ Resistor, 8Ω Speaker.
* **Features:** Web playback controls, volume slider, LittleFS partition setup, fallback Wi-Fi AP.

#### ⚠️ Problems Faced & Solutions
- **Problem:** No pin markings on the top of the ESP32 board, making breadboard pin connections difficult to map.
  - **Solution:** Referred to the official NodeMCU-32 pinout diagram and counted physical pins relative to the micro-USB port.
- **Problem:** Audio was extremely quiet when connecting the speaker directly to the ESP32 DAC pin (GPIO 25), and didn't know an audio amplifier circuit was needed.
  - **Solution:** Added an S8050 NPN transistor amplification circuit (driven by GPIO 25 through a 1kΩ resistor) to boost current to the 8Ω speaker.
- **Problem:** Audio static / pop noise when ESP32 powers on.
  - **Solution:** Initialized audio objects lazily on playback request so GPIO 25 stays quiet during Wi-Fi startup.

---

### 2. ESP32 Screen LED Control (`esp32-screen-led-control`)
An interactive web dashboard with custom dark-mode UI to control an onboard/external LED with real-time brightness adjustment and visual effects.

* **URL:** `http://esp32LED.local`
* **Key Components:** ESP32 DevKit, Built-in LED (GPIO 2).
* **Features:** Modern UI, PWM brightness slider, 3x blink animation, REST API status endpoints.

#### ⚠️ Problems Faced & Solutions
- **Problem:** No pin markings on top of the ESP32 module, making it hard to identify GPIO 2 once plugged into the breadboard.
  - **Solution:** Checked the pinout diagram to locate GPIO 2 and verified with the onboard LED before wiring an external component.

---

### 3. ESP32 Button Counter (`esp32-button-web-dashboard`)
A straightforward hardware project demonstrating physical button inputs and event logging over Serial.

* **Key Components:** ESP32 DevKit, Pushbutton (GPIO 18).
* **Features:** Internal pull-up resistor usage, Serial monitor logging at 115200 baud.

#### ⚠️ Problems Faced & Solutions
- **Problem:** Top of the ESP32 lacks pin labels, causing confusion when identifying GPIO 18 vs GND pins on the breadboard.
  - **Solution:** Counted pin offsets carefully from the pinout documentation.

---

### 4. ESP32 Motor Control Hub (`esp32-motor-control`)
An interactive web dashboard with hardware driver routines to operate DC motors (via L298N/L293D H-Bridge driver) and Servo motors with real-time speed, direction, and angle controls.

* **URL:** `http://esp32motor.local`
* **Key Components:** ESP32 DevKit, L298N / L293D Motor Driver, DC Motor, SG90 Servo Motor.
* **Features:** H-bridge direction control (Forward/Reverse/Stop), PWM speed slider (0-100%), Servo angle slider (0-180°), REST API endpoints (`/api/motor`, `/api/servo`, `/api/status`), SoftAP fallback.

#### ⚠️ Problems Faced & Solutions
- **Problem:** ESP32 resets unexpectedly when the motor starts running due to voltage drops.
  - **Solution:** Powered the motor driver with an external battery/power supply, keeping power supplies separate while sharing a common ground connection.
- **Problem:** Servo jitter during Wi-Fi activity.
  - **Solution:** Used ESP32 hardware LEDC PWM timer at 50Hz with 16-bit pulse resolution for stable pulse width timing.

---
## ⚡ Quick Start (PlatformIO)

1. Clone or open any project directory in VS Code with PlatformIO installed.
2. Edit `.env` inside the project folder with your Wi-Fi credentials:
   ```env
   WIFI_SSID="Your_WiFi_Name"
   WIFI_PASSWORD="Your_WiFi_Password"
   ```
3. Build and upload code to ESP32:
   ```bash
   pio run --target upload
   ```
4. Open the Serial Monitor to verify connection:
   ```bash
   pio device monitor
   ```
