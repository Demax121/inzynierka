/*
=====================================================================
 Lux Sensor Node (ESP32-C3 super mini + VEML7700 + AES WS)
=====================================================================
Hardware wiring (I2C):
  VIN  -> 5V (board regulator supplies 3V3 to sensor internally)
  3V0  -> not used (could power sensor directly if breakout supports)
  GND  -> GND
  SCL  -> GPIO9
  SDA  -> GPIO8

Functional overview:
  * Initializes VEML7700 ambient light sensor (gain 1, 100ms integration)
  * Connects to WiFi (MyWiFi helper) and WebSocket backend channel 'lux_sensor'
  * Reads lux continuously and transmits encrypted updates only when change >= LUX_THRESHOLD
  * Responds to plaintext server 'ping' by sampling & sending current lux
  * Uses AES-128-CBC encryption (random IV per message) inside an envelope JSON
  * Basic watchdog attempts reconnect if socket appears stale beyond timeout

Performance notes:
  - Lower integration time yields faster but noisier readings; threshold helps suppress noise bursts
  - Consider averaging or exponential smoothing if jitter becomes an issue

Potential improvements:
  - Deep sleep with periodic wake if power budget critical
  - Adaptive threshold (e.g., percentage based for high lux values)
  - Include sensor status / error flags in payload
*/


// ===================================================================
//  Includes
// ===================================================================
#include <Wire.h>
#include <Adafruit_VEML7700.h>
#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <AESCrypto.h>
#include <WiFiClientSecure.h>
#include "certs.h" // for root CA if using wss://

// ===================================================================
//  1. Piny / Podłączenie (Pins / Hardware)
// ===================================================================
#define SDA_PIN 8
#define SCL_PIN 9

// ===================================================================
//  2. Zmienne (Variables / Configuration & State)
// ===================================================================
// Network / WS
String WEBSOCKET_SERVER = "websocket.simplysmart.duckdns.org"; // Backend Bun server
const int WEBSOCKET_PORT = 443;                // Port
const unsigned int WEBSOCKET_RECONNECT_INTERVAL = 5000; // Library auto reconnect interval (ms)
const unsigned long WS_RECONNECT_TIMEOUT = 15000;        // Manual watchdog stale threshold
const unsigned long WS_RETRY_EVERY = 5000;               // Manual retry interval
String device_api_key = "9ekJU68REYTR";                  // Device API key
String encryption_key = "J247J3LBDegpAUaU";              // 16-char AES key

// Sensor / thresholds
const int LUX_THRESHOLD = 10;                 // Minimum change to trigger transmission
int lastLux = -999;                           // Last sent lux
bool sensorReady = false;                     // Sensor init flag

// Objects
WebSocketsClient webSocketClient;             // WS client
Adafruit_VEML7700 veml;                       // VEML7700 driver
StaticJsonDocument<256> jsonPayload;          // (Placeholder) envelope cache
WiFiClientSecure clientSSL;                   // TLS client
AESCrypto crypto(encryption_key);             // AES helper

// Watchdog timing
unsigned long lastWsConnected = 0;            // Last connected timestamp
unsigned long lastWsAttempt = 0;              // Last manual reconnect attempt

// ===================================================================
//  3. Funkcje (Functions)
// ===================================================================

// (Reserved) Initialize JSON envelope; currently unused beyond structural placeholder
void initializeJSON() {
}

// Update jsonPayload with latest lux reading (if envelope pattern is later reused)
void updateJSONData(int currentLux) {
  JsonObject payload = jsonPayload["payload"]; // may be null if not pre-created
  payload["lux"] = currentLux;
}

// Send identification frame (plaintext) so server associates socket with device
void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "lux_sensor";
  idDoc["device_api_key"] = device_api_key;
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocketClient.sendTXT(idMessage);
}

// Encrypt current lux value and transmit to server
void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  StaticJsonDocument<96> p;
  p["lux"] = lastLux;
  String plain; serializeJson(p, plain);
  String nonce = AESCrypto::generateNonce();
  String cipherHex, tagHex; if(!crypto.encrypt(plain, nonce, cipherHex, tagHex)) return;
  StaticJsonDocument<192> env;
  env["identity"] = "lux_sensor";
  env["channel"] = "lux_sensor";
  env["device_api_key"] = device_api_key;
  env["nonce"] = nonce;
  env["payload"] = cipherHex;
  env["tag"] = tagHex;
  String out; serializeJson(env, out);
  webSocketClient.sendTXT(out);
}

// Handle inbound WS text frames (currently only plaintext 'ping')
void handleWebSocketMessage(uint8_t* payload, size_t length) {
  StaticJsonDocument<128> doc;
  String msg = "";
  for (size_t i = 0; i < length; i++) msg += (char)payload[i];
  DeserializationError err = deserializeJson(doc, msg);
  if (err) return;
  // Handle ping message from server
  if (doc.containsKey("type") && String((const char*)doc["type"]) == "ping") {
    int currentLux = sensorReady ? (int)veml.readLux() : -1;
    updateJSONData(currentLux);
    sendWebSocketData();
  }
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

// Setup: initialize serial, WiFi, I2C, sensor, initial lux sample, WebSocket connection
void setup() {
  Serial.begin(19200);
  MyWiFi::connect();



  Wire.begin(SDA_PIN, SCL_PIN);

  sensorReady = veml.begin();
  if (sensorReady) {
    veml.setGain(VEML7700_GAIN_1);
    veml.setIntegrationTime(VEML7700_IT_100MS);
    veml.powerSaveEnable(false);
  }

  initializeJSON();
  int initialLux = sensorReady ? (int)veml.readLux() : -1;
  updateJSONData(initialLux);
  lastLux = initialLux;

  webSocketClient.beginSSL(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
  webSocketClient.onEvent([](WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_CONNECTED) {
      lastWsConnected = millis();
      identifyDevice();
    } else if (type == WStype_TEXT) {
      handleWebSocketMessage(payload, length);
    }
  });

    lastWsConnected = millis();
  lastWsAttempt = millis();
}

// Main loop: service WS, sample sensor, send if threshold crossed, watchdog reconnect
void loop() {
  webSocketClient.loop();

  int currentLux = sensorReady ? (int)veml.readLux() : -1;

  if (abs(currentLux - lastLux) >= LUX_THRESHOLD) {
    updateJSONData(currentLux);
    sendWebSocketData();
    lastLux = currentLux;
  }

  handleWebSocketWatchdog();
}
