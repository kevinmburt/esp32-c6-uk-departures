SwiftUI iOS app (scaffold) - ESP Configurator

Instructions:
1. Create a new iOS SwiftUI project in Xcode (iPhone target >= iOS 15).
2. Add the files from this folder (BLEManager.swift, ContentView.swift) to the project.
3. Add "Privacy - Bluetooth Always Usage Description" and "Privacy - Bluetooth Peripheral Usage Description" to Info.plist with user-visible strings.
4. Run on a physical device (CoreBluetooth not available in Simulator for BLE central operations).

The app scans for the BLE service UUID and writes a JSON configuration to the writable characteristic.
