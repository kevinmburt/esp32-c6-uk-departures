import Foundation
import CoreBluetooth
import Combine

class BLEManager: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    @Published var isScanning = false
    @Published var peripherals: [CBPeripheral] = []
    @Published var connectedPeripheral: CBPeripheral?

    private var centralManager: CBCentralManager!
    private var targetServiceUUID = CBUUID(string: "12345678-1234-5678-1234-56789abcdef0")
    private var configCharUUID = CBUUID(string: "12345678-1234-5678-1234-56789abcdef1")
    private var configCharacteristic: CBCharacteristic?

    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            print("BLE powered on")
        } else {
            print("BLE state: \(central.state.rawValue)")
        }
    }

    func startScan() {
        peripherals = []
        isScanning = true
        centralManager.scanForPeripherals(withServices: [targetServiceUUID], options: nil)
    }

    func stopScan() {
        centralManager.stopScan()
        isScanning = false
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        if !peripherals.contains(peripheral) {
            peripherals.append(peripheral)
        }
    }

    func connect(_ peripheral: CBPeripheral) {
        centralManager.connect(peripheral, options: nil)
        peripheral.delegate = self
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        print("Connected to \(peripheral.name ?? "unknown")")
        connectedPeripheral = peripheral
        peripheral.discoverServices([targetServiceUUID])
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let services = peripheral.services else { return }
        for s in services {
            peripheral.discoverCharacteristics([configCharUUID], for: s)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let characteristics = service.characteristics else { return }
        for c in characteristics {
            if c.uuid == configCharUUID {
                configCharacteristic = c
                print("Found config characteristic")
            }
        }
    }

    func sendConfig(ssid: String, pass: String, appId: String, appKey: String, station: String) -> Bool {
        guard let peripheral = connectedPeripheral, let char = configCharacteristic else { return false }
        let dict: [String: String] = ["ssid": ssid, "pass": pass, "app_id": appId, "app_key": appKey, "station": station]
        do {
            let data = try JSONSerialization.data(withJSONObject: dict, options: [])
            peripheral.writeValue(data, for: char, type: .withResponse)
            return true
        } catch {
            print("Config serialize error: \(error)")
            return false
        }
    }
}
