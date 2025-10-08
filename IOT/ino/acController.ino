/*
=====================================================================
 Air Conditioning Controller (ESP32 + ILI9341 TFT + WebSocket + AES)
=====================================================================
Hardware wiring (SPI TFT ILI9341):
  LED/BACKLIGHT -> 3V3 (may be PWM'ed externally if dimming desired)
  SCK          -> GPIO18 (HSPI SCK)
  MOSI (SDI)   -> GPIO23
  DC           -> GPIO21 (data/command select)
  RESET        -> GPIO22 (can tie to EN with pull-up if you want auto reset)
  CS           -> GPIO5  (chip select)
  GND          -> GND
  VCC          -> 3V3
  MISO (SDO)   -> not used (read operations ignored)

User input:
  Momentary button (normally open) between GPIO13 and GND (internal pull-up used).

Notes:
  - If the board exposes only a single 3V3 pin, fan out both TFT VCC and LED/BACKLIGHT from it.
  - Touch controller not used (omit related wiring).

Functional overview:
  * Connect to WiFi (MyWiFi helper library abstracting credentials)
  * Establish WebSocket to backend server (Bun) and identify on channel 'air_conditioning'
  * Exchange encrypted JSON payloads (AES-128-CBC) for status + commands
  * Display current temperature, requested temperature, on/off state, and function (heating / cooling) on TFT
  * Local hysteresis control: if difference to requested temperature >= HISTERAZA, choose heating/cooling; else idle
  * Manual override: button toggles klimaOn state; cancels function until automatic logic re-engages
  * Periodically re-sends state when ping received, or when any local/remote change occurs

Security & Encryption:
  - Each outbound payload is AES encrypted with per-device key (encryption_key) and random IV
  - Inbound messages expected to include msgIV + payload (hex) used for decryption

Reconnect strategy:
  - Library's internal reconnect + manual watchdog: if disconnected too long, explicit restart of connection

Code style:
  - Minimal dynamic allocation: StaticJsonDocument used for deterministic memory
  - Strings kept as global 'String' for simplicity; consider migrating to constexpr char[] to reduce heap churn
*/

#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <AESCrypto.h>
#include <AESCrypto.h>

// ===================================================================
//  1. Piny / Podłączenie (Pins / Hardware mapping)
// ===================================================================
#define TFT_CS   5
#define TFT_DC   21
#define TFT_RST  22


// ===================================================================
//  2. Zmienne (Variables / Configuration & State)
// ===================================================================
const int buttonPin = 13;           // GPIO for manual override button
bool BTNstate = false;              // (Unused residual variable; kept for future expansion)

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

WebSocketsClient webSocketClient;

String WEBSOCKET_SERVER = "192.168.1.2";          // Backend WS host (LAN IP)
const int   WEBSOCKET_PORT   = 8884;               // Backend WS port
const unsigned long RECONNECT_INTERVAL = 5000;     // Library auto reconnect interval (ms)
String device_api_key = "NfzziMcV9Zyj";            // Device API key (server uses to map encryption key & update last_seen)
String encryption_key = "KQAzhJmqigFdUyD6";       // 16-char AES key (AES-128)

bool klimaOn = false;               // Current HVAC state (relay active / compressor running)
bool manualOverride = false;        // If true, automatic hysteresis adjustments are suspended

float currentTemp = 0.0;            // Last received ambient temperature (0 => not yet received)
float requestedTemp = 25.0;         // Target temperature set by frontend
const float HISTERAZA = 1.0;        // Hysteresis threshold (°C) before toggling cooling / heating
String currentFunction = "";       // Display label: cooling / heating / idle

AESCrypto crypto(encryption_key);   // AES helper instance (CBC mode with random IV)

unsigned long lastWsConnected = 0;          // Timestamp of last successful connection
unsigned long lastWsAttempt = 0;            // Timestamp of last manual reconnect attempt
const unsigned long WS_RECONNECT_TIMEOUT = 15000; // If stale this long, escalate reconnect

StaticJsonDocument<256> doc;                 // For inbound decoded (outer) JSON structures
StaticJsonDocument<512> jsonPayload;         // Prepared outbound envelope (payload field mutated per send)

// UI label strings (Polish localization)
// UI Strings (English)
String label_current_temp   = "CURRENT TEMP:";      // Current temperature label
String label_target_temp    = "TARGET TEMP:";       // Requested temperature label
String label_status         = "STATUS:";            // On/off status label
String label_function       = "FUNCTION:";          // Function label (cool / heat / idle)
String on_text              = "ON";
String off_text             = "OFF";
String idle_text            = "Idle Mode";          // Idle status
String space_c              = " C";                 // Unit suffix
String dash_c               = "-- C";               // Placeholder before first reading
String cooling_mode_text    = "Cooling Mode";       // Cooling label
String heating_mode_text    = "Heating Mode";       // Heating label

// Initialize TFT display layout and static labels
void initializeDisplay() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  
  tft.setCursor(10, 20);
  tft.print(label_current_temp);
  tft.setCursor(10, 100);
  tft.print(label_target_temp);
  tft.setCursor(10, 180);
  tft.print(label_status);
  tft.setCursor(160, 180);
  tft.print(label_function);
}

// Refresh dynamic fields (temperature values, on/off status, function)
void updateDisplay() {
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);
  
  tft.fillRect(10, 50, 300, 30, ILI9341_BLACK);
  tft.setCursor(10, 50);
  if (currentTemp > 0.0) {
    tft.print(currentTemp, 1);
    tft.print(space_c);
  } else {
    tft.print(dash_c);
  }
  
  tft.fillRect(10, 130, 300, 30, ILI9341_BLACK);
  tft.setCursor(10, 130);
  tft.print(requestedTemp, 1);
  tft.print(space_c);
  
  tft.fillRect(10, 210, 140, 25, ILI9341_BLACK);
  tft.setCursor(10, 210);
  tft.setTextSize(2);
  if (klimaOn) {
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.print(on_text);
  } else {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.print(off_text);
  }
  
  tft.fillRect(160, 210, 150, 25, ILI9341_BLACK);
  tft.setCursor(160, 210);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setTextSize(2);
  if (currentFunction.length() > 0) {
    tft.print(currentFunction);
  } else {
  tft.print(idle_text);
  }
}

// Placeholder reserved for future preallocation / structure init
void initializeJSON() {
  // envelope created on send (currently no-op)
}

// ===================================================================
//  3. Funkcje (Functions)
// ===================================================================

// Populate jsonPayload's payload object with current device state
void updateJSONData() {
  JsonObject payload = jsonPayload["payload"];
  payload["requestedTemp"] = requestedTemp;
  payload["function"] = currentFunction;
  payload["klimaON"] = klimaOn;
  payload["manualOverride"] = manualOverride;
}

// Send identification frame so server associates this socket with device_api_key & channel
void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "air_conditioning";
  idDoc["device_api_key"] = device_api_key;
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocketClient.sendTXT(idMessage);
}

// Encrypt and send current AC state to server (channel: air_conditioning)
void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  updateJSONData();
  StaticJsonDocument<256> p;
  p["requestedTemp"] = requestedTemp;
  p["function"] = currentFunction;
  p["klimaON"] = klimaOn;
  p["manualOverride"] = manualOverride;
  String plain; serializeJson(p, plain);
  String nonce = AESCrypto::generateNonce(); String cipherHex, tagHex; if(!crypto.encrypt(plain, nonce, cipherHex, tagHex)) return;
  StaticJsonDocument<320> env;
  env["identity"] = "air_conditioning";
  env["channel"] = "air_conditioning";
  env["device_api_key"] = device_api_key;
  env["nonce"] = nonce;
  env["payload"] = cipherHex;
  env["tag"] = tagHex;
  String out; serializeJson(env, out);
  webSocketClient.sendTXT(out);
  Serial.printf("[TX] AC %s\n", klimaOn ? "Włączone" : "Wyłączone");
}

// Decide whether to heat / cool / idle based on hysteresis & user override
void checkTemperatureControl() {
  if (currentTemp == 0) return;
  if (manualOverride) return;
  
  float tempDiff = currentTemp - requestedTemp;
  bool previousState = klimaOn;
  String previousFunction = currentFunction;
  if (tempDiff >= HISTERAZA) {
    klimaOn = true;
    currentFunction = cooling_mode_text;
    if (!previousState || previousFunction != currentFunction) {
      Serial.println("CHŁODZIMY!!!");
      sendWebSocketData();
      updateDisplay();
    }
  } else if (tempDiff <= -HISTERAZA) {
    klimaOn = true;
    currentFunction = heating_mode_text;
    if (!previousState || previousFunction != currentFunction) {
      Serial.println("GRZEJEMY!!!");
      sendWebSocketData();
      updateDisplay();
    }
  } else {
    if (klimaOn) {
      klimaOn = false;
  currentFunction = idle_text;
      Serial.println("Temperatura OK - wyłączam klimę");
      sendWebSocketData();
      updateDisplay();
    }
  }
}

// Evaluate currentFunction based purely on current temperature vs requestedTemp (ignores manualOverride)
void evaluateFunctionByTemp() {
  if (!klimaOn) { // If AC is off -> idle
    currentFunction = idle_text;
    return;
  }
  if (currentTemp == 0) { // No reading yet
    currentFunction = idle_text;
    return;
  }
  float diff = currentTemp - requestedTemp;
  if (diff >= HISTERAZA) {
    currentFunction = cooling_mode_text;
  } else if (diff <= -HISTERAZA) {
    currentFunction = heating_mode_text;
  } else {
    currentFunction = idle_text;
  }
}

// Parse inbound WebSocket text frames (outer JSON may be plaintext envelope with encrypted 'payload')
void handleIncomingText(uint8_t* payload, size_t length) {
  String msg = "";
  for (size_t i = 0; i < length; i++) msg += (char)payload[i];
  DeserializationError err = deserializeJson(doc, msg);
  if (err) return;
  
  // Handle ping message from server
  if (doc.containsKey("type") && String((const char*)doc["type"]) == "ping") {
    sendWebSocketData();
    return;
  }

  String channel = doc["channel"] | "";
  if (channel != "air_conditioning") return;

  // Encrypted payloads from server for both temperature push and commands (GCM)
  if (!doc.containsKey("nonce") || !doc.containsKey("payload") || !doc.containsKey("tag")) return;
  String nonce = doc["nonce"].as<String>();
  String cipherHex = doc["payload"].as<String>();
  String tagHex = doc["tag"].as<String>();
  String plain = crypto.decrypt(cipherHex, nonce, tagHex);
  if (plain.length() == 0) return;
  StaticJsonDocument<256> body; if (deserializeJson(body, plain)) return;

  // If contains temperature (room_stats push)
  if (body.containsKey("temperature")) {
    float t = body["temperature"].as<float>();
    currentTemp = t;
    Serial.printf("[air_conditioning] Current temperature: %.1f°C\n", t);
    updateDisplay();
    checkTemperatureControl();
  }

  // Frontend commands
  if (body.containsKey("klimaON")) {
    bool newState = body["klimaON"];
    if (newState != klimaOn) {
      klimaOn = newState;
      if (body.containsKey("manualOverride")) {
        manualOverride = body["manualOverride"];
      }
      if (!newState) {
        currentFunction = idle_text;
      } else {
        // Always evaluate function when turning ON (even in manual override)
        evaluateFunctionByTemp();
        // If not manual override, allow automatic hysteresis decisions next cycles
        if (!manualOverride) {
          checkTemperatureControl();
        }
      }
      updateDisplay();
      sendWebSocketData();
      Serial.printf("[RX] AC %s (manual control)\n", klimaOn ? "ON" : "OFF");
    }
  }

  if (body.containsKey("requestedTemp")) {
    float reqTemp = body["requestedTemp"].as<float>();
    requestedTemp = reqTemp;
    manualOverride = false;
    Serial.printf("[air_conditioning] Requested temperature: %.1f°C\n", reqTemp);
    updateDisplay();
    checkTemperatureControl();
    sendWebSocketData();
  }
}

// Debounced button handling (toggles klimaOn, sets manualOverride)
void handleButton() {
  static bool lastButton = HIGH;
  static unsigned long lastButtonChange = 0;
  const unsigned long BUTTON_DEBOUNCE_MS = 200;
  
  bool reading = digitalRead(buttonPin);
  
  if (reading == LOW && lastButton == HIGH) {
    if (millis() - lastButtonChange > BUTTON_DEBOUNCE_MS) {
      klimaOn = !klimaOn;
      manualOverride = true;
      if (!klimaOn) {
        currentFunction = idle_text;
      } else {
        // Determine initial function immediately based on temperature difference
        evaluateFunctionByTemp();
      }
      Serial.printf("Button pressed - AC %s (manual)\n", klimaOn ? "ON" : "OFF");
      updateDisplay();
      if (webSocketClient.isConnected()) {
        sendWebSocketData();
      }
      lastButtonChange = millis();
    }
  }

  lastButton = reading;
}

// Arduino setup: initializes serial, IO, WiFi, WebSocket, display
void setup() {
  Serial.begin(19200);
  delay(500);
  pinMode(buttonPin, INPUT_PULLUP);
  initializeDisplay();
  WiFi.setHostname("esp32_ac_controller");
  MyWiFi::connect();

  initializeJSON();

  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  lastWsConnected = millis();
  lastWsAttempt = millis();
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      lastWsConnected = millis();
      Serial.println("Connected to WS – identifying air_conditioning channel");
      identifyDevice();
      sendWebSocketData();
      updateDisplay();
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
    }
  });
  webSocketClient.setReconnectInterval(RECONNECT_INTERVAL);
}

// Watchdog: monitor stale connection & manually force reconnect
void websocketWatchdog() {
  if (webSocketClient.isConnected()) return;
  unsigned long now = millis();
  const unsigned long RETRY_EVERY = 5000;
  if (now - lastWsConnected > WS_RECONNECT_TIMEOUT && now - lastWsAttempt > RETRY_EVERY) {
    Serial.println("WebSocket not responding, restarting connection...");
    webSocketClient.disconnect();
    delay(100);
    webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
    lastWsAttempt = now;
  }
}

// Main loop: button handling, WebSocket service, reconnect watchdog
void loop() {
  // Handle button at the beginning - independent of WebSocket
  handleButton();
  
  // WebSocket loop - may block during reconnect
  MyWiFi::loop();
  webSocketClient.loop();

  if (MyWiFi::isConnected())
  {
    websocketWatchdog();
  }
  
  
}

