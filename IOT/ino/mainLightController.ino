/*
NODEMCU ESP32S + 2 WAY 5V RELAY  HW-383A 5V + TOUCH BUTTON

HW-383A 5V -> ESP32S:
GND - GND
INT1 - PIN27
INT2 - NONE
VCC - 5V

TOUCH BUTTON -> ESP32S:
GND - GND
i/O - PIN22
VCC - 3V3

*/


#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <AESCrypto.h>

WebSocketsClient webSocketClient;

const int RELAY_PIN = 27;
const int TOUCH_BUTTON_PIN = 22;
const int RELAY_ACTIVE_LEVEL = LOW;
const int RELAY_INACTIVE_LEVEL = HIGH;
const int BUTTON_ACTIVE_LEVEL = HIGH;
const unsigned int BUTTON_DEBOUNCE_MS = 50;

bool State = false;
int lastButtonState = LOW;
int buttonState = LOW;
unsigned int lastButtonChangeMs = 0;

String WEBSOCKET_SERVER = "192.168.1.2";
const int WEBSOCKET_PORT = 8884;
const unsigned int WEBSOCKET_RECONNECT_INTERVAL = 5000;
String device_api_key = "kZ8UQmdrDar8";
String encryption_key = "Vfyu3xT6e6yy79iE";


AESCrypto crypto(encryption_key);


// Watchdog WebSocket
unsigned long lastWsConnected = 0;
unsigned long lastWsAttempt = 0;
const unsigned long WS_RECONNECT_TIMEOUT = 15000;


StaticJsonDocument<256> jsonPayload; // envelope only used on send

void initializeJSON() { 
  // envelope created on send
}

void updateJSONData(bool lightState) { 
  JsonObject payload = jsonPayload["payload"];
  payload["lightON"] = lightState;
}

void setRelay(bool on) { 
  State = on; 
  digitalWrite(RELAY_PIN, on ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL); 
  updateJSONData(on);
}

void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "main_lights";
  idDoc["device_api_key"] = device_api_key;
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocketClient.sendTXT(idMessage);
}

void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  StaticJsonDocument<96> p; p["lightON"] = State; String plain; serializeJson(p, plain);
  String iv = AESCrypto::generateIV(); String cipher = crypto.encrypt(plain, iv);
  StaticJsonDocument<192> env;
  env["identity"] = "lights_controller";
  env["channel"] = "main_lights";
  env["device_api_key"] = device_api_key;
  env["msgIV"] = iv;
  env["payload"] = cipher;
  String out; serializeJson(env, out);
  webSocketClient.sendTXT(out);
}

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
  if (!doc.containsKey("msgIV") || !doc.containsKey("payload")) return;
  String iv = doc["msgIV"].as<String>();
  String cipher = doc["payload"].as<String>();
  String plain = crypto.decrypt(cipher, iv);
  if (plain.length() == 0) return;
  StaticJsonDocument<128> cmd; if (deserializeJson(cmd, plain)) return;
  if (!cmd.containsKey("lightON")) return;
  bool lightON = cmd["lightON"].as<bool>();
  setRelay(lightON);
  sendWebSocketData();
}

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
  if (!webSocketClient.isConnected()) {
    if (millis() - lastWsConnected > WS_RECONNECT_TIMEOUT && millis() - lastWsAttempt > 5000) {
      Serial.println("WebSocket nie odpowiada, restart połączenia...");
      webSocketClient.disconnect();
      delay(100);
  webSocketClient.begin(WEBSOCKET_SERVER.c_str(), WEBSOCKET_PORT, "/");
      lastWsAttempt = millis();
    }
  }
}