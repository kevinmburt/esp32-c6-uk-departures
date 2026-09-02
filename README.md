# esp32-c6-uk-departures

ESP32-C6 project to display UK National Rail departures on a 1.47" ST7789 display with iPhone provisioning.

This repository contains:
- firmware/: Arduino C++ firmware for ESP32-C6 (BLE provisioning, Wi‑Fi, TransportAPI fetch, ST7789 display)
- ios/: SwiftUI iPhone configurator app (scans BLE, sends Wi‑Fi + TransportAPI keys + station code)

Quick start summary

1. Get a TransportAPI app_id/app_key: https://developer.transportapi.com/
2. Wire the 1.47" ST7789 display to your ESP32-C6 (pins in firmware can be adjusted in the sketch).
3. Build and flash the Arduino sketch (see firmware/README.md for details).
4. Open the iOS app project in Xcode, run on your device, scan and connect to the ESP32 named "ESP32-Departures", enter Wi‑Fi credentials, TransportAPI keys, and station CRS (three‑letter code, e.g., LBG).

If you prefer Network Rail Darwin API instead of TransportAPI, update the URL and parsing in the firmware accordingly.
