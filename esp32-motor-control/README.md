# ESP32 Motor Control Hub ⚙️

An interactive hardware driver and real-time Web Dashboard for operating **DC Motors** (via L298N / L293D H-Bridge driver) and **Servo Motors** (via PWM) using the ESP32 microcontroller.

---

## 📌 Features

* **Dual Motor Drivers:** PWM speed control + Direction switching for DC motors, plus 0–180° positioning for Servo motors.
* **Modern Web Dashboard:** Responsive dark-mode UI with live telemetry, directional buttons, and smooth sliders.
* **REST API Endpoints:** Complete API interface for external automation or custom app integration.
* **Smart Wi-Fi Setup:** Automatically connects to your home Wi-Fi or boots a fallback Access Point (`ESP32-Motor-Network`).
* **mDNS Support:** Accessible directly via `http://esp32motor.local`.

---

## 🔌 Hardware & Pinout Mapping

| Component | ESP32 Pin | Description |
| :--- | :--- | :--- |
| **L298N ENA** | `GPIO 25` | DC Motor PWM Speed Control |
| **L298N IN1** | `GPIO 26` | DC Motor Direction Input 1 |
| **L298N IN2** | `GPIO 27` | DC Motor Direction Input 2 |
| **Servo Signal** | `GPIO 19` | Servo PWM Pulse Signal (SG90 / MG996R) |
| **GND** | `GND` | Common Ground (ESP32 + Driver GND) |

> ⚠️ **CRITICAL POWER WARNING:** Motors draw high current spikes that will reset or damage the ESP32 if powered directly from the 3.3V/5V pins. **Always use an external power supply (e.g., 6V - 12V battery or wall adapter) for the motor driver**, and connect the external power supply GND to the ESP32 GND.

---

## 🔌 Where to Plug in the 2 Motor Ends (Wires)

### Option 1: L298N Motor Driver Module (Speed + Direction Control)
* **Motor Wire 1 (End 1):** Screw into **`OUT1`** on the L298N module.
* **Motor Wire 2 (End 2):** Screw into **`OUT2`** on the L298N module.
* **L298N Control Wires:**
  - **Remove the ENA jumper** from the L298N module, then connect `ENA` -> ESP32 **GPIO 25** (Speed / PWM).
  - `IN1` -> ESP32 **GPIO 26** (Direction 1)
  - `IN2` -> ESP32 **GPIO 27** (Direction 2)
  - `GND` -> ESP32 **GND** & Power Supply **GND (-)**

> **Startup safety:** Add a **10kΩ pull-down resistor** between `ENA` and `GND`. It keeps the driver disabled during the short interval before the ESP32 firmware takes control of GPIO 25. Never test with the motor mechanically loaded or able to move something unexpectedly.

---

### Option 2: NPN Transistor / MOSFET (Simple 2-Wire ON / OFF Switch)
* **Motor Wire 1 (End 1):** Connect to **Power Positive (+)** (Battery `+` or ESP32 `VIN` for small 5V motors).
* **Motor Wire 2 (End 2):** Connect to Transistor **Collector Pin** (Middle leg of NPN S8050 / TIP120) or MOSFET Drain.
* **Transistor Control Wires:**
  - **Base Pin (Left leg):** Connect through a **1kΩ Resistor** to ESP32 **GPIO 25**.
  - **Emitter Pin (Right leg):** Connect to **GND** (ESP32 GND & Battery `-`).

---

## 📡 REST API Documentation

| Endpoint | Method | Query Parameters | Description |
| :--- | :--- | :--- | :--- |
| `/` | `GET` | — | Serves the web dashboard interface |
| `/api/status` | `GET` | — | Returns JSON telemetry and current motor states |
| `/api/motor` | `GET` | `dir=FORWARD\|REVERSE\|STOP`, `speed=0..100` | Controls DC motor direction & speed |
| `/api/servo` | `GET` | `angle=0..180` | Sets servo motor angle |

### Example API Request
```bash
# Drive DC motor forward at 75% speed
curl "http://esp32motor.local/api/motor?dir=FORWARD&speed=75"

# Set servo angle to 90 degrees
curl "http://esp32motor.local/api/servo?angle=90"
```

---

## ⚡ Quick Start & Build Instructions

1. Open this folder in VS Code with the **PlatformIO** extension installed.
2. Edit `.env` with your local Wi-Fi credentials:
   ```env
   WIFI_SSID="Your_WiFi_Name"
   WIFI_PASSWORD="Your_WiFi_Password"
   ```
3. Connect your ESP32 board via USB.
4. Upload code & view Serial Monitor:
   ```bash
   pio run --target upload
   pio device monitor
   ```
5. Open your browser and navigate to `http://esp32motor.local` (or `http://192.168.4.1` if connected via SoftAP).

---

## ⚠️ Problems Faced & Solutions

- **Problem:** ESP32 resets when the DC motor turns on.
  - **Solution:** Powered the motor driver (L298N VCC/GND) from an external power supply instead of the ESP32 5V pin, and tied grounds together.
- **Problem:** Jittery servo motor movements when running Wi-Fi.
  - **Solution:** Configured dedicated ESP32 hardware LEDC PWM channel (50 Hz, 16-bit resolution) to ensure pulse timing stays rock solid during Wi-Fi packet transmission.
