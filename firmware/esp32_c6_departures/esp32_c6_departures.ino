#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "NimBLEDevice.h"

// ----------------------
// Display pins - adjust as needed for your module
#define TFT_CS     5
#define TFT_DC     16
#define TFT_RST    17

// If your display is 320x172 swap width/height
#define TFT_WIDTH  240
#define TFT_HEIGHT 135

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ----------------------
// BLE
#define BLE_DEVICE_NAME "ESP32-Departures"
static NimBLEServer* pServer = nullptr;
static NimBLECharacteristic* pConfigChar = nullptr;

// UUIDs
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CONFIG_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1"

// ----------------------
// Preferences storage
Preferences preferences;

// Config keys
String wifi_ssid;
String wifi_pass;
String transport_app_id;
String transport_app_key;
String station_code; // CRS code

// Runtime
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL_MS = 60000; // 60s

// Forward declarations
void startBLE();
void connectWiFiIfConfigured();
void fetchAndDisplay();

class ConfigCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar) {
    std::string value = pChar->getValue();
    if (value.length() == 0) return;
    Serial.println("Received config via BLE:");
    Serial.println(value.c_str());

    // Expecting a JSON blob like {"ssid":"...","pass":"...","app_id":"...","app_key":"...","station":"..."}
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, value);
    if (err) {
      Serial.print("JSON parse failed: "); Serial.println(err.c_str());
      return;
    }

    if (doc.containsKey("ssid")) wifi_ssid = String((const char*)doc["ssid"]);
    if (doc.containsKey("pass")) wifi_pass = String((const char*)doc["pass"]);
    if (doc.containsKey("app_id")) transport_app_id = String((const char*)doc["app_id"]);
    if (doc.containsKey("app_key")) transport_app_key = String((const char*)doc["app_key"]);
    if (doc.containsKey("station")) station_code = String((const char*)doc["station"]);

    // store
    preferences.putString("ssid", wifi_ssid);
    preferences.putString("pass", wifi_pass);
    preferences.putString("app_id", transport_app_id);
    preferences.putString("app_key", transport_app_key);
    preferences.putString("station", station_code);

    // Try to connect to WiFi now
    connectWiFiIfConfigured();
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Init preferences
  preferences.begin("departures", false);
  wifi_ssid = preferences.getString("ssid", "");
  wifi_pass = preferences.getString("pass", "");
  transport_app_id = preferences.getString("app_id", "");
  transport_app_key = preferences.getString("app_key", "");
  station_code = preferences.getString("station", "");

  // Init display
  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  // Draw placeholder
  tft.setCursor(0, 0);
  tft.println("ESP32-C6 UK Departures");
  tft.println();
  tft.println("Waiting for configuration...");

  // Start BLE to receive config
  startBLE();

  // If WiFi already configured, try connect
  connectWiFiIfConfigured();
}

void loop() {
  // Periodically fetch departures if we have config and WiFi
  if (WiFi.status() == WL_CONNECTED && station_code.length() > 0 && transport_app_id.length() > 0 && transport_app_key.length() > 0) {
    if (millis() - lastUpdate > UPDATE_INTERVAL_MS) {
      fetchAndDisplay();
      lastUpdate = millis();
    }
  }

  delay(100);
}

void startBLE() {
  NimBLEDevice::init(BLE_DEVICE_NAME);
  pServer = NimBLEDevice::createServer();
  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  pConfigChar = pService->createCharacteristic(
    CONFIG_CHAR_UUID,
    NIMBLE_PROPERTY::WRITE
  );
  pConfigChar->setCallbacks(new ConfigCallback());

  pService->start();
  NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
  pAdv->addServiceUUID(SERVICE_UUID);
  pAdv->setScanResponse(true);
  pAdv->start();

  Serial.println("BLE advertising started");
}

void connectWiFiIfConfigured() {
  if (wifi_ssid.length() == 0) {
    Serial.println("No WiFi SSID stored");
    return;
  }

  if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == wifi_ssid) {
    Serial.println("Already connected to WiFi");
    return;
  }

  Serial.printf("Connecting to WiFi '%s'...\n", wifi_ssid.c_str());
  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    // Clear display area
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0,0);
    tft.println("WiFi connected");
    tft.println(WiFi.localIP().toString());
  } else {
    Serial.println("WiFi connect failed");
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0,0);
    tft.println("WiFi connect failed");
  }
}

void fetchAndDisplay() {
  if (transport_app_id.length() == 0 || transport_app_key.length() == 0 || station_code.length() == 0) {
    Serial.println("Missing API keys or station");
    return;
  }

  String url = "/v3/uk/train/station/" + station_code + "/live.json?app_id=" + transport_app_id + "&app_key=" + transport_app_key + "&darwin=false&train_status=passenger";

  // TransportAPI supports https. We'll use HTTPClient with TLS disabled verification for simplicity.
  HTTPClient http;
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();
  http.begin(*client, String("https://transportapi.com") + url);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET failed, code: %d\n", httpCode);
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0,0);
    tft.println("Fetch failed");
    tft.printf("HTTP %d", httpCode);
    http.end();
    delete client;
    return;
  }

  String payload = http.getString();
  http.end();
  delete client;

  Serial.println("Received payload");
  // Parse JSON
  DynamicJsonDocument doc(20*1024);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("JSON parse error: "); Serial.println(err.c_str());
    return;
  }

  // TransportAPI: departures -> all
  JsonObject departures = doc["departures"];
  if (departures.isNull()) {
    Serial.println("No departures node");
    return;
  }

  JsonArray all = departures["all"].as<JsonArray>();

  // Display header
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0,0);
  tft.setTextSize(1);
  tft.println("Station: " + station_code);
  tft.println();

  int lines = 0;
  const int maxLines = 5;
  for (JsonObject entry : all) {
    if (lines >= maxLines) break;
    const char* dest = entry["destination_name"] | "";
    const char* aimed = entry["aimed_departure_time"] | "";
    const char* expected = entry["expected_departure_time"] | "";
    const char* platform = entry["platform"] | "";

    String line = String(aimed) + " ";
    if (String(expected) != String(aimed)) {
      line += String(expected) + " ";
    }
    line += " ";
    if (platform && strlen(platform)>0) {
      line += "P" + String(platform) + " ";
    }
    line += String(dest);

    tft.println(line);
    lines++;
  }

  if (lines == 0) {
    tft.println("No upcoming departures found");
  }

}
