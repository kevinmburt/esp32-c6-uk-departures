# Firmware

This folder contains the Arduino sketch for the ESP32-C6.

Requirements
- Arduino IDE or PlatformIO with ESP32 C6 support
- Libraries:
  - NimBLE-Arduino
  - ArduinoJson
  - Adafruit GFX
  - Adafruit ST7789

Notes
- Pins for the ST7789 are configurable at the top of the sketch. Adjust to match your board and breakout wiring.
- The sketch exposes a BLE service named "ESP32 Departures" with a writable configuration characteristic. The iOS app writes a JSON object to that characteristic with keys: ssid, pass, app_id, app_key, station.

