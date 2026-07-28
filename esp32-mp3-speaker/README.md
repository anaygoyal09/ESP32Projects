# ESP32 MP3 Web Controller & Speaker Driver

An interactive web controller running on an ESP32 (**`http://esp32.local`**) to control MP3 audio playback through a transistor-driven speaker.

---

## 📐 Hardware Wiring

```text
                 [ NodeMCU-32 ESP32 ]
                   5V/VIN     P25(DAC1)     GND
                     │            │          │
                     │         [1kΩ Res]     │
                     │            │          │
                     │            ▼          │
 (Red + Wire)        │       (Base / Pin 2)  │
      ┌──────────────┴──┐         │          │
    [ 2-Prong Speaker ]           ▼          │
      └──────────────┬──┘     ┌───────┐      │
 (Black - Wire)      │        │ NPN   │      │
                     └───────►│ Trans │      │
                (Collector)   └───┬───┘      │
                  (Pin 3)         │          │
                                  ▼          │
                             (Emitter)       │
                              (Pin 1)        │
                                  └──────────┴───► GND
```

### 📌 Pin Connections
1. **S8050 Emitter (Pin 1)** ➔ ESP32 `GND`
2. **S8050 Base (Pin 2)** ➔ **1kΩ Resistor** ➔ ESP32 `P25` (DAC1)
3. **S8050 Collector (Pin 3)** ➔ Speaker **Black (-)**
4. **Speaker Red (+)** ➔ ESP32 `5V / VIN`

---

## ⚡ Quick Setup & Upload

1. Set your Wi-Fi details in `.env`:
   ```env
   WIFI_SSID="YOUR_WIFI_NAME"
   WIFI_PASSWORD="YOUR_WIFI_PASSWORD"
   ```
2. Upload to ESP32:
   ```bash
   pio run --target upload
   ```
3. Visit **`http://esp32.local`** in your browser *(or fallback AP `ESP32-Audio-Network` / `192.168.4.1`)*.

---

## ⚠️ Problems Faced & Solutions

- **Problem:** No pin labels printed on top of the ESP32 board, making it hard to identify GPIO pins on the breadboard.
  - **Solution:** Referenced the NodeMCU-32 pinout diagram online and counted physical pin counts from the top edge.
- **Problem:** Audio was extremely quiet when plugging the speaker directly into the ESP32, and wasn't aware an amplifier/transistor circuit was necessary.
  - **Solution:** Wired an S8050 NPN transistor circuit with a 1kΩ base resistor to amplify current to the speaker, greatly boosting volume.
- **Problem:** Audio static / pop noise when ESP32 powers on.
  - **Solution:** Initialized audio objects only on playback request so GPIO 25 stays quiet during Wi-Fi startup.
