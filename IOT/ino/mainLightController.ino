/*
=====================================================================
 Main Lights Controller (NodeMCU ESP32S + HW-383A 5V dual relay + touch button)
=====================================================================
Relay module wiring (using only channel 1):
  GND   -> GND
  IN1   -> GPIO27 (active LOW logic)
  IN2   -> not used
  VCC   -> 5V (module optocoupler / transistor supply)

Touch button (capacitive / digital module):
  GND   -> GND
  I/O   -> GPIO22 (reads HIGH when touched depending on module polarity)
  VCC   -> 3V3

Functional overview:
  * Provides local physical toggle of main lights via touch sensor
  * Connects to backend via WebSocket channel 'main_lights'
  * Sends and receives encrypted state frames (AES-128-CBC) inside JSON envelope
  * Debounces the touch input to prevent rapid false toggles
  * Responds to server 'ping' by re-sending current state
  * Watchdog monitors stale connection and forces reconnect if necessary

Safety / hardware considerations:
  - RELAY_ACTIVE_LEVEL defined as LOW (module is low-level trigger) -> adjust if hardware differs
  - Ensure appropriate isolation and safety for mains switching beyond relay board (not shown here)

Potential improvements:
  - Long press detection for alternate scene
  - Double-tap to force broadcast even if unchanged
  - Power-on state restoration from non-volatile memory
*/


#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <AESCrypto.h>

WebSocketsClient webSocketClient;

const int RELAY_PIN = 27;                  // Relay control GPIO (channel 1)
const int TOUCH_BUTTON_PIN = 22;           // Capacitive touch / digital input
const int RELAY_ACTIVE_LEVEL = LOW;        // Module triggers on LOW (adjust if different board)
const int RELAY_INACTIVE_LEVEL = HIGH;     // Idle relay level
const int BUTTON_ACTIVE_LEVEL = HIGH;      // Touch module outputs HIGH when activated
const unsigned int BUTTON_DEBOUNCE_MS = 50;// Debounce threshold (ms)

bool State = false;                        // Current logical light state
int lastButtonState = LOW;                 // Last sampled raw reading
int buttonState = LOW;                     // Debounced stable state
unsigned int lastButtonChangeMs = 0;       // Timestamp of last edge (for debounce timing)

String WEBSOCKET_SERVER = "192.168.1.2";          // Backend WS server
const int WEBSOCKET_PORT = 8884;                    // WS port
const unsigned int WEBSOCKET_RECONNECT_INTERVAL = 5000; // Library-managed reconnect interval
String device_api_key = "kZ8UQmdrDar8";            // Device API key (maps to devices table / encryption key)
String encryption_key = "Vfyu3xT6e6yy79iE";       // 16-char AES key


AESCrypto crypto(encryption_key);                // AES helper instance


// WebSocket watchdog timestamps
unsigned long lastWsConnected = 0;                 // Last successful connect time
unsigned long lastWsAttempt = 0;                   // Last manual reconnect attempt
const unsigned long WS_RECONNECT_TIMEOUT = 15000;  // Stale threshold


StaticJsonDocument<256> jsonPayload;              // Optional envelope cache (payload mutated per send)

// Prepare JSON scaffold (currently no-op; kept for symmetry with other nodes)
void initializeJSON() { }

// Update payload portion of envelope with current light state
void updateJSONData(bool lightState) { 
  JsonObject payload = jsonPayload["payload"]; // may allocate on first access
  payload["lightON"] = lightState;
}

// Drive relay hardware & update internal state + JSON cache
void setRelay(bool on) { 
  State = on; 
  digitalWrite(RELAY_PIN, on ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL); 
  updateJSONData(on);
}

// Send identification frame so server links socket -> device & channel
void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "main_lights";
  idDoc["device_api_key"] = device_api_key;
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocketClient.sendTXT(idMessage);
}

// Encrypt and transmit current light state to server
void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  StaticJsonDocument<96> p; p["lightON"] = State; String plain; serializeJson(p, plain);
  String nonce = AESCrypto::generateNonce(); String cipherHex, tagHex; if(!crypto.encrypt(plain, nonce, cipherHex, tagHex)) return;
  StaticJsonDocument<192> env;
  env["identity"] = "lights_controller";
  env["channel"] = "main_lights";
  env["device_api_key"] = device_api_key;
  env["nonce"] = nonce;
  env["payload"] = cipherHex;
  env["tag"] = tagHex;
  String out; serializeJson(env, out);
  webSocketClient.sendTXT(out);
}

// Parse and act on inbound WS frames; supports plaintext 'ping' + encrypted lightON commands
void handleIncomingText(uint8_t* payload, size_t length) {
  if (!payload || length == 0) return;
  StaticJsonDocument<128> doc;
  String msg = "";
  for (size_t i = 0; i < length; i++) msg += (char)payload[i];
  DeserializationError err = deserializeJson(doc, msg);
  if (err) return;
  // Handle ping message from server
  if (doc.containsKey("type") && String((const char*)doc["type"]) == "ping") {
    updateJSONData(State);
    sendWebSocketData();
    return;
  }
  // Encrypted command from server: { channel:"main_lights", msgIV, payload } with payload { lightON }
  String ch = doc["channel"] | "";
  if (ch != "main_lights") return;
  if (!doc.containsKey("nonce") || !doc.containsKey("payload") || !doc.containsKey("tag")) return;
  String nonce = doc["nonce"].as<String>();
  String cipherHex = doc["payload"].as<String>();
  String tagHex = doc["tag"].as<String>();
  String plain = crypto.decrypt(cipherHex, nonce, tagHex);
  if (plain.length() == 0) return;
  StaticJsonDocument<128> cmd; if (deserializeJson(cmd, plain)) return;
  if (!cmd.containsKey("lightON")) return;
  bool lightON = cmd["lightON"].as<bool>();
  setRelay(lightON);
  sendWebSocketData();
}

// Setup: IO config, WiFi connect, WS connect, identify, initial state broadcast
void setup() {
  Serial.begin(19200);
  digitalWrite(RELAY_PIN, RELAY_INACTIVE_LEVEL);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TOUCH_BUTTON_PIN, INPUT);
  initializeJSON();
  setRelay(false);
  MyWiFi::connect();
  webSocketClient.begin(WEBSOCKET_SERVER.c_str(), WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      lastWsConnected = millis(); // aktualizuj czas połączenia
      identifyDevice();
      sendWebSocketData();
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
    lastWsConnected = millis();
  lastWsAttempt = millis();
}

// Watchdog: monitor stale connection & manually force reconnect
void websocketWatchdog() {
  if (webSocketClient.isConnected()) return;
  unsigned long now = millis();
  const unsigned long RETRY_EVERY = 5000;
  if (now - lastWsConnected > WS_RECONNECT_TIMEOUT && now - lastWsAttempt > RETRY_EVERY) {
    Serial.println("WebSocket nie odpowiada, restart połączenia...");
    webSocketClient.disconnect();
    delay(100);
    webSocketClient.begin(WEBSOCKET_SERVER.c_str(), WEBSOCKET_PORT, "/");
    lastWsAttempt = now;
  }
}

// Main loop: debounce touch button, service WS, force broadcast on local toggle, watchdog reconnect
void loop() {
  webSocketClient.loop();

  // Czytaj aktualny stan przycisku
  int reading = digitalRead(TOUCH_BUTTON_PIN);
  
  // Sprawdź czy stan się zmienił (reset timer debounce)
  if (reading != lastButtonState) {
    lastButtonChangeMs = millis();
  }
  
  // Jeśli minął czas debounce, zaktualizuj stan
  if ((millis() - lastButtonChangeMs) > BUTTON_DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      
      // Jeśli przycisk został naciśnięty (przejście LOW -> HIGH)
      if (buttonState == BUTTON_ACTIVE_LEVEL) {
        setRelay(!State);
        sendWebSocketData();
      }
    }
  }
  
  lastButtonState = reading;



    // Watchdog WebSocket
  websocketWatchdog();
}