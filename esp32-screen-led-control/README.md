# ESP32 Screen Click LED Control

An interactive web dashboard running on an ESP32 (**`http://esp32LED.local`**) to toggle an LED, adjust PWM brightness, and run light effects directly from a web browser.

---

## ⚡ Quick Setup

1. Configure `.env`:
   ```env
   WIFI_SSID="Your_WiFi_Name"
   WIFI_PASSWORD="Your_WiFi_Password"
   ```
2. Upload to ESP32:
   ```bash
   pio run -t upload
   ```
3. Open `http://esp32LED.local` in your browser *(or fallback AP `ESP32-LED-Control` / `192.168.4.1`)*.

---

## 🔌 Hardware Setup

- **ESP32 DevKit**
- **Onboard LED:** GPIO 2
- *(Optional)* External LED connected to GPIO 2 via 220Ω resistor to GND.

---

## ⚠️ Problems Faced & Solutions

- **Problem:** No pin labels printed on the top of the ESP32 board, making connection and finding GPIO 2 difficult on the breadboard.
  - **Solution:** Cross-referenced the ESP32 pinout diagram to identify GPIO 2 and GND positions.
