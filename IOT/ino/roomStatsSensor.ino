/*
=====================================================================
 Room Statistics Sensor (ESP32-C3 + BME280 + AES WebSocket)
=====================================================================
Sensor note:
  BME280 provides temperature, humidity, and pressure. (BMP280 lacks humidity.)
  Some breakout boards are dual-labeled BME/BMP 280; ensure actual BME280 variant.

Wiring (I2C):
  VIN  -> 5V
  GND  -> GND
  SCL  -> GPIO5
  SDA  -> GPIO4

Functional overview:
  * Initializes BME280 over I2C
  * Connects to backend WS channel 'room_stats' and identifies
  * Periodically (loop-driven) samples sensor; transmits only when temperature delta >= TEMPERATURE_THRESHOLD
  * Responds to plaintext 'ping' messages by sending an immediate encrypted stats frame
  * Encrypts each payload using AES-128-CBC with random IV (shared key with server)
  * Watchdog triggers manual reconnect if connection stale beyond threshold

Design decisions:
  - Threshold applies only to temperature (primary driver of change). Humidity/pressure changes alone won't trigger send; acceptable for dashboard context.
  - Reads sensor fresh for each send rather than caching between updateJSONData & send to avoid drift; still slight time gap negligible.
  - StaticJsonDocument used for deterministic memory footprint.

Possible improvements:
  - Multi-metric threshold (OR/AND humidity or pressure change)
  - Periodic heartbeat even if unchanged (e.g., every 5 min) for monitoring
  - Deep sleep + timer wake to save power if battery-powered
*/


// ===================================================================
//  Includes
// ===================================================================
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <AESCrypto.h>
#include <WiFiClientSecure.h>
#include "certs.h" // for root CA if using wss://

// ===================================================================
//  1. Piny / Podłączenie (Pins / Hardware)
// ===================================================================
#define SDA_PIN 4                               // I2C SDA
#define SCL_PIN 5                               // I2C SCL
const uint8_t BME280_I2C_ADDRESS = 0x76;        // BME280 address

// ===================================================================
//  2. Zmienne (Variables / Configuration & State)
// ===================================================================
// Network / WS
String WEBSOCKET_SERVER = "websocket.simplysmart.duckdns.org"; // Backend server
const int WEBSOCKET_PORT = 443;                   // Port
const unsigned int WEBSOCKET_RECONNECT_INTERVAL = 5000; // Library auto reconnect interval (ms)
const unsigned long WS_RECONNECT_TIMEOUT = 15000;        // Manual stale threshold
const unsigned long WS_RETRY_EVERY = 5000;               // Manual retry interval
String device_api_key = "b3odiEjCNSf7";                  // Device API key
String encryption_key = "xuCmb33pFJgJAwR5";              // 16-char AES key

// Sensor / thresholds
const int TEMPERATURE_THRESHOLD = 1;             // Minimum Δ°C to send
float lastTemperature = -100.0;                  // Last sent temperature (sentinel)

// Objects
Adafruit_BME280 bme;                             // Sensor driver
WebSocketsClient webSocketClient;                // WS client
StaticJsonDocument<256> jsonPayload;             // Envelope cache
WiFiClientSecure clientSSL;                      // TLS client
AESCrypto crypto(encryption_key);                // AES helper

// Watchdog timing
unsigned long lastWsConnected = 0;               // Last connected timestamp
unsigned long lastWsAttempt = 0;                 // Last manual attempt

// ===================================================================
//  3. Funkcje (Functions)
// ===================================================================

// Prepare JSON scaffold (currently no-op; retained for consistency across device sketches)
void initializeJSON() { }

// Populate payload object with latest sensor readings (fresh sampled each call)
void updateJSONData() {
  JsonObject payload = jsonPayload["payload"];
  payload["temperature"] = (float)bme.readTemperature();
  payload["humidity"] = (float)bme.readHumidity();
  payload["pressure"] = (float)(bme.readPressure() / 100.0); // Pa -> hPa
}

// Send identification frame so server associates socket with device & channel
void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "room_stats";
  idDoc["device_api_key"] = device_api_key;
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocketClient.sendTXT(idMessage);
}

// Encrypt and send current sensor snapshot to server
void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  StaticJsonDocument<192> p;
  p["temperature"] = (float)bme.readTemperature();
  p["humidity"] = (float)bme.readHumidity();
  p["pressure"] = (float)(bme.readPressure() / 100.0);
  String plain; serializeJson(p, plain);
  String nonce = AESCrypto::generateNonce(); String cipherHex, tagHex; if(!crypto.encrypt(plain, nonce, cipherHex, tagHex)) return;
  StaticJsonDocument<256> env;
  env["identity"] = "room_stats_sensor";
  env["channel"] = "room_stats";
  env["device_api_key"] = device_api_key;
  env["nonce"] = nonce;
  env["payload"] = cipherHex;
  env["tag"] = tagHex;
  String out; serializeJson(env, out);
  webSocketClient.sendTXT(out);
}

// Watchdog: monitor stale WS connection & attempt manual reconnect
void handleWebSocketWatchdog() {
  if (webSocketClient.isConnected()) return;
  unsigned long now = millis();
  if (now - lastWsConnected > WS_RECONNECT_TIMEOUT && now - lastWsAttempt > WS_RETRY_EVERY) {
    Serial.println("WebSocket nie odpowiada, restart połączenia...");
    webSocketClient.disconnect();
    delay(100);
    webSocketClient.beginSSL(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
    lastWsAttempt = now;
  }
}

// Setup: serial, WiFi, I2C, sensor init, initial sample, WS connect + handlers
void setup() {
  Serial.begin(19200);
  MyWiFi::connect();
  Wire.begin(SDA_PIN, SCL_PIN);
  bme.begin(BME280_I2C_ADDRESS);
  initializeJSON();
  
  // Pobierz początkową temperaturę w formacie int
  lastTemperature = (float)bme.readTemperature();
  updateJSONData();
  
  webSocketClient.beginSSL(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      lastWsConnected = millis(); // aktualizuj czas połączenia
      identifyDevice();
      updateJSONData();
      sendWebSocketData();
    }
    else if (type == WStype_TEXT) {
      // Handle ping from server
      StaticJsonDocument<128> doc;
      String msg = "";
      for (size_t i = 0; i < length; i++) msg += (char)payload[i];
      DeserializationError err = deserializeJson(doc, msg);
      if (!err && doc.containsKey("type") && String((const char*)doc["type"]) == "ping") {
        updateJSONData();
        sendWebSocketData();
      }
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
    lastWsConnected = millis();
  lastWsAttempt = millis();
}

// Main loop: WS service, evaluate temperature delta, watchdog reconnect
void loop() {
  webSocketClient.loop();
  
  // Sample current temperature (float precision retained)
  float currentTemperature = (float)bme.readTemperature();
  
  // If temperature change exceeds threshold, broadcast updated stats
  if (fabs(currentTemperature - lastTemperature) >= TEMPERATURE_THRESHOLD) {
    updateJSONData();
    sendWebSocketData();
    lastTemperature = currentTemperature;
  }

  handleWebSocketWatchdog();
}
