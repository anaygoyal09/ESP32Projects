# ESP32 Button Counter

A simple PlatformIO project for an ESP32 DevKit and a pushbutton, reporting button clicks over Serial at 115200 baud.

---

## 🔌 Hardware Connections

- **ESP32 DevKit board**
- **Pushbutton:** Connect one pin to **GPIO 18** and the other pin to **GND** (uses internal pull-up resistor).

---

## ⚡ Build & Run

```bash
pio run --target upload
pio device monitor
```

---

## ⚠️ Problems Faced & Solutions

- **Problem:** No pin labels on the top of the ESP32 chip, making it hard to connect GPIO 18 and GND accurately on the breadboard.
  - **Solution:** Referred to the pinout documentation diagram and counted pin positions.
