# ESP32 Addressable LED Strip Web Controller

PlatformIO project for a **5 V, 3-wire addressable LED strip** (such as WS2812B or SK6812) hosted on HTTP port 80 at **[http://esp32.local](http://esp32.local)**.

> **Note on single LED lighting:** In the starter code, `LED_COUNT` was set to `1` as a safety default for initial testing. The web interface now allows you to dynamically adjust the active LED count slider from **1 to 300 LEDs** live without changing code or re-flashing!

---

## Connections

| Strip marking | Connect to |
| --- | --- |
| `5V`, `VCC`, or `+` | Regulated 5 V supply positive |
| `GND`, `-`, or `G` | 5 V supply ground and ESP32 `GND` |
| `DIN`, `DI`, or `DATA IN` | ESP32 GPIO 5 through a 220–470 ohm resistor |

The arrows printed on the strip must point **away from `DIN`** toward `DOUT`. Connect the ESP32 data wire to the end marked `DIN`/`DI`.

```text
External regulated 5V +  ---- strip 5V / +
External regulated 5V -  ---- strip GND / - ---- ESP32 GND
ESP32 GPIO 5 --------[330 ohm]---- strip DIN / DI
```

---

## Quick Setup & Wi-Fi Configuration

1. Copy `.env.example` to `.env`:
   ```bash
   cp .env.example .env
   ```
2. Open `.env` and set your Wi-Fi network credentials:
   ```env
   WIFI_SSID="Your_WiFi_Name"
   WIFI_PASSWORD="Your_WiFi_Password"
   ```
3. If home Wi-Fi credentials are missing or the connection times out, the ESP32 starts a direct-control access point:
   - **SSID:** `ESP32-LED-Strip`
   - **Password:** `password123`
   - **IP:** `http://192.168.4.1`

`esp32.local` works when your phone/computer and ESP32 are on the same compatible network. If home Wi-Fi cannot be reached, connect your phone/computer directly to `ESP32-LED-Strip` and open `http://192.168.4.1`. No `:80` suffix is required because HTTP port 80 is the default.

---

## Web Controller Features (`http://esp32.local`)

Once connected, navigate to **[http://esp32.local](http://esp32.local)** in any web browser to control the strip:

- **Active LED Count Slider (1 to 300):** Adjust to match the exact length of your LED strip. Quick preset buttons for 1, 10, 30, 60, 120, and 300 LEDs.
- **Color Picker & Palette:** Custom HEX color picker + RGB inputs + 8 quick-select color presets (Red, Green, Blue, Purple, Orange, Cyan, Warm White, Cool White).
- **Brightness Slider (0 - 100%):** Smooth brightness scaling.
- **Lighting Effects:**
  - `Solid Color`
  - `Rainbow Wave`
  - `Breathing Pulse`
  - `Color Wipe`
  - `Color Chase`
  - `Twinkle / Fire`
- **Effect Speed Slider:** Control animation transition rate (10ms – 200ms).
- **Power Switch:** Toggle strip on/off with state preservation.

---

## REST API Endpoints

- `GET /api/status` - Returns current JSON state (power, RGB, hex, brightness, active count, max count, mode, speed, IP, uptime).
- `POST /api/control` (or `GET /api/control`) - Updates settings live:
  - Query/Body Parameters: `power`, `r`, `g`, `b`, `brightness`, `count`, `mode`, `speed`

---

## Uploading Firmware

```bash
pio run
pio run --target upload
pio device monitor
```

This board is configured to upload through the ESP32 ROM bootloader because its temporary compressed-upload stub stops responding when the application image begins. A full upload takes roughly 90 seconds at 115200 baud; keep the USB cable still and wait for `Hash of data verified` and `SUCCESS`.
