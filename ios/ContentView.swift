import SwiftUI

struct ContentView: View {
    @StateObject var ble = BLEManager()
    @State private var selectedPeripheral: Int? = nil
    @State private var ssid = ""
    @State private var password = ""
    @State private var appId = ""
    @State private var appKey = ""
    @State private var station = ""

    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Detected devices")) {
                    if ble.peripherals.isEmpty {
                        Text("No devices found").foregroundColor(.secondary)
                    } else {
                        ForEach(0..<ble.peripherals.count, id: \ .self) { idx in
                            let p = ble.peripherals[idx]
                            HStack {
                                Text(p.name ?? "Unknown")
                                Spacer()
                                Button("Connect") {
                                    ble.connect(p)
                                }
                            }
                        }
                    }
                    Button(ble.isScanning ? "Stop scan" : "Start scan") {
                        if ble.isScanning { ble.stopScan() } else { ble.startScan() }
                    }
                }

                Section(header: Text("Wi-Fi & API")) {
                    TextField("SSID", text: $ssid)
                    SecureField("Password", text: $password)
                    TextField("TransportAPI app_id", text: $appId)
                    TextField("TransportAPI app_key", text: $appKey)
                    TextField("Station CRS (e.g. LBG)", text: $station)
                }

                Section {
                    Button("Send config to device") {
                        let success = ble.sendConfig(ssid: ssid, pass: password, appId: appId, appKey: appKey, station: station)
                        print("Sent: \(success)")
                    }
                }
            }
            .navigationTitle("ESP Configurator")
        }
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
