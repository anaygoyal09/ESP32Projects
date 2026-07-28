# ESP32 MP3 Web Controller & Speaker Driver (http://esp32.local)

This project runs directly inside **`esp32-mp3-speaker`** (using the same architecture as `esp32-screen-led-control`). It serves an interactive web dashboard at **`http://esp32.local`** where clicking the interactive speaker icon or buttons starts, stops, and controls volume for MP3 audio played through your **NodeMCU-32** and **S8050 NPN transistor**!

---

## 📐 Breadboard Wiring Diagram (S8050 Transistor + Speaker)

```
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

---

## 📌 Pin Connections

1. **S8050 Emitter (Leg 1)** ➔ Connect to **ESP32 `GND`**
2. **S8050 Base (Leg 2)** ➔ Connect through a **1kΩ Resistor** to **ESP32 `P25`**
3. **S8050 Collector (Leg 3)** ➔ Connect to **Speaker Black Wire (-)**
4. **Speaker Red Wire (+)** ➔ Connect to **ESP32 `5V / VIN`**

---

## ⚡ Quick Setup & Upload

### 1. Configure Wi-Fi Credentials
Open `.env` in this directory and set your network credentials:
```env
WIFI_SSID="YOUR_WIFI_NAME"
WIFI_PASSWORD="YOUR_WIFI_PASSWORD"
```

### 2. Upload Code to ESP32
Run the standard PlatformIO upload command:
```bash
pio run --target upload
```

---

## 🌐 Opening the Website (`http://esp32.local`)

1. Make sure your phone or computer is on the **same Wi-Fi network**.
2. Open your browser and go to:  
   👉 **`http://esp32.local`**
3. You can now click the **Speaker Icon** or **Start MP3** / **Stop MP3** / **Volume Slider** to control sound playback live!

*(Note: If Wi-Fi fails to connect, the ESP32 automatically creates a fallback Wi-Fi hotspot named `ESP32-Audio-Network` with password `password123`. After joining it, use `http://esp32.local` or, if your device does not support mDNS, `http://192.168.4.1`.)*
