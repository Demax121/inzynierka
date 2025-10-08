/*
=====================================================================
 Door Sensor Node (ESP32-C3 super mini + limit / reed switch + AES WS)
=====================================================================
Hardware wiring:
  Button / switch contacts:
    - Common (C)     -> GND
    - Normally Open  -> GPIO21 (internal pull-up enabled)
    - Normally Closed -> not used

  The switch closes to ground -> logic LOW while pressed/closed, HIGH when open
  We invert logical meaning in updateJSONData: doorOpen = (buttonState == HIGH)
  so that HIGH (released) => door open, LOW (closed circuit) => door closed.

Functional overview:
  * Connects to WiFi using MyWiFi helper (non-blocking maintenance in loop)
  * Establishes WebSocket to backend server (Bun) on channel 'door_sensor'
  * Sends encrypted payloads with doorOpen boolean using AES-128-CBC
  * Responds to server 'ping' (unencrypted) by re-sending current state
  * Debounces physical switch with simple time-based filter (DEBOUNCE_MS)
  * Includes watchdog reconnect if WebSocket becomes unresponsive for an extended period

Security:
  - Outbound frames: envelope {identity, channel, device_api_key, msgIV, payload}
  - payload is ciphertext of { doorOpen: bool }
  - AES key + API key provisioned statically here (improve by moving to secure storage or provisioning step in future)

Possible improvements (future):
  - Low-power sleep & wake-on-change (ULP / ext0) to reduce energy usage
  - Tamper detection (rapid state toggling) -> alert
  - Heartbeat every N minutes even if unchanged
*/
// ===================================================================
//  Includes
// ===================================================================
#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Arduino.h>
#include <esp_random.h>
#include <AESCrypto.h>
#include <WiFiClientSecure.h>
#include "certs.h" // for root CA if using wss://

// ===================================================================
//  1. Piny / Podłączenie (Pins & Hardware)
// ===================================================================
const int BUTTON_PIN = 21;                 // Door switch input (internal pull-up)

// ===================================================================
//  2. Zmienne (Variables / Configuration & State)
// ===================================================================
// Network & encryption
String WEBSOCKET_SERVER =" websocket server domain name"; // Backend server
const int WEBSOCKET_PORT = 443;                 // WebSocket port
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000; // Library auto reconnect (ms)
const unsigned long WS_RECONNECT_TIMEOUT = 15000;        // Stale threshold
const unsigned long WS_RETRY_EVERY = 5000;               // Manual retry interval
String device_api_key = "akr3ZPYDU5rM";                  // Device API key
String encryption_key = "2zBxk8uBiQd7p32T";             // 16-byte AES key

// Debounce
int lastButtonState = HIGH;                 // Last stable state
const unsigned long DEBOUNCE_MS = 30;       // Debounce threshold
unsigned long lastChangeTime = 0;           // Potential change start timestamp

// Runtime state
bool doorOpen = false;                      // Logical door state

// WS / crypto objects
WebSocketsClient webSocketClient;           // WS client
WiFiClientSecure clientSSL;                 // TLS client
AESCrypto crypto(encryption_key);           // AES helper

// Watchdog timing
unsigned long lastWsConnected = 0;          // Last connect event
unsigned long lastWsAttempt = 0;            // Last manual attempt

// ===================================================================
//  3. Funkcje (Functions)
// ===================================================================
// Initialize state / JSON scaffolding (currently simple flag reset)
void initializeJSON() { 
  doorOpen = false; // explicit init
}

// Translate raw pin reading to logical door state
// HIGH (pull-up not grounded) => circuit open => door physically open
// LOW  (pulled to ground)     => switch closed => door closed
void updateJSONData(int buttonState) { 
  doorOpen = (buttonState == HIGH);
}

// Send identification frame so server maps socket -> device & updates last_seen
void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "door_sensor";
  idDoc["device_api_key"] = device_api_key;
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocketClient.sendTXT(idMessage);
}

// Encrypt and transmit current door state to backend server
void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  // Build clear payload
  StaticJsonDocument<128> p;
  p["doorOpen"] = doorOpen;
  String plain; serializeJson(p, plain);
  String nonce = AESCrypto::generateNonce();
  String cipherHex, tagHex;
  if (!crypto.encrypt(plain, nonce, cipherHex, tagHex)) return;
  StaticJsonDocument<256> env;
  env["identity"] = "main_door_sensor";
  env["channel"] = "door_sensor";
  env["device_api_key"] = device_api_key;
  env["nonce"] = nonce;
  env["payload"] = cipherHex;
  env["tag"] = tagHex;
  String out; serializeJson(env, out);
  webSocketClient.sendTXT(out);
}

// Process inbound WebSocket frames (handles only plaintext 'ping' currently)
void handleIncomingText(uint8_t* payload, size_t length) {
  String msg = "";
  for (size_t i = 0; i < length; i++) msg += (char)payload[i];
  
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) return;
  
  // Handle ping message from server (unencrypted)
  if (doc.containsKey("type") && String((const char*)doc["type"]) == "ping") {
    updateJSONData(digitalRead(BUTTON_PIN));
    sendWebSocketData();
  }
  
  // Handle other unencrypted messages if needed in the future
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

// Arduino setup: initialize serial, WiFi, IO, WS client
void setup() {
  Serial.begin(19200);
  
  MyWiFi::connect();
  // dalsze próby łączenia obsłuży MyWiFi::loop() w pętli głównej
  clientSSL.setCACert(root_ca);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  initializeJSON();
  lastButtonState = digitalRead(BUTTON_PIN);
  updateJSONData(lastButtonState);
  webSocketClient.beginSSL(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      lastWsConnected = millis(); // aktualizuj czas połączenia
      identifyDevice();
      int currentButtonState = digitalRead(BUTTON_PIN);
      updateJSONData(currentButtonState);
      sendWebSocketData();
      lastButtonState = currentButtonState;
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
  lastWsConnected = millis();
  lastWsAttempt = millis();
}

// Main loop: maintain WiFi, debounce switch, service WebSocket, watchdog reconnect
void loop() {
  // Utrzymanie połączenia WiFi (nieblokujące)
  MyWiFi::loop();

  // Jeśli nie ma WiFi, nie próbuj obsługiwać WebSocket ani odczytu zdarzeń (redukcja hałasu)
  if (!MyWiFi::isConnected()) {
    delay(10);
    return;
  }

  webSocketClient.loop();
  int currentState = digitalRead(BUTTON_PIN);
  if (currentState != lastButtonState) {
    if (lastChangeTime == 0) {
      lastChangeTime = millis();
    } else if (millis() - lastChangeTime >= DEBOUNCE_MS) {
      lastButtonState = currentState;
      updateJSONData(lastButtonState);
      sendWebSocketData();
      lastChangeTime = 0;
    }
  } else {
    lastChangeTime = 0;
  }

  handleWebSocketWatchdog();
}
