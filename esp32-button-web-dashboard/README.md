# ESP32 Button

Simple PlatformIO project for an ESP32 DevKit and a pushbutton. Each press is
reported in the serial monitor.

## Hardware

- ESP32 DevKit board
- Pushbutton
- USB data cable

Connect one side of the pushbutton to GPIO 18 and the other side to GND. The
program uses the ESP32's internal pull-up resistor, so no external resistor is
needed.

Open the serial monitor at 115200 baud. Each time the button is pressed, the
program prints `Button clicked`.

## Build and upload

1. Install VS Code and the PlatformIO extension.
2. Open this folder as a PlatformIO project.
3. Connect the ESP32 by USB.
4. Run **Build**, then **Upload**.
5. Open the serial monitor at **115200 baud**.

Command-line equivalent:

```text
pio run
pio run --target upload
pio device monitor
```
