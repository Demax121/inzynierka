#include <MyWiFiV2.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

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
const int WEBSOCKET_PORT = 8886;
const unsigned int WEBSOCKET_RECONNECT_INTERVAL = 5000;


// Watchdog WebSocket
unsigned long lastWsConnected = 0;
unsigned long lastWsAttempt = 0;
const unsigned long WS_RECONNECT_TIMEOUT = 15000;


StaticJsonDocument<256> jsonPayload;

void initializeJSON() { 
  jsonPayload["identity"] = "lights_controller";
  jsonPayload["channel"] = "main_lights";
  JsonObject payload = jsonPayload.createNestedObject("payload");
  payload["lightON"] = false;
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
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocketClient.sendTXT(idMessage);
}

void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  String jsonStr;
  serializeJson(jsonPayload, jsonStr);
  webSocketClient.sendTXT(jsonStr);
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
  if (!doc.containsKey("lightON")) return;
  if (doc.containsKey("channel") && String((const char*)doc["channel"]) != "main_lights") return;
  bool lightON = doc["lightON"];
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
  MyWiFiV2::connect();
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