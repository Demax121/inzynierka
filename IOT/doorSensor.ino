#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

String WEBSOCKET_SERVER = "192.168.1.2";
const int WEBSOCKET_PORT = 8886;
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;

const int BUTTON_PIN = 21;
int lastButtonState = HIGH;
const unsigned long DEBOUNCE_MS = 30;
unsigned long lastChangeTime = 0;

WebSocketsClient webSocketClient;
StaticJsonDocument<256> jsonPayload;
bool doorOpen = false;

// Watchdog WebSocket
unsigned long lastWsConnected = 0;
unsigned long lastWsAttempt = 0;
const unsigned long WS_RECONNECT_TIMEOUT = 15000;

void initializeJSON() { 
  jsonPayload["identity"] = "main_door_sensor";
  jsonPayload["channel"] = "door_sensor";
  JsonObject payload = jsonPayload.createNestedObject("payload");
  payload["doorOpen"] = false;
}

void updateJSONData(int buttonState) { 
  doorOpen = (buttonState == HIGH);
  JsonObject payload = jsonPayload["payload"];
  payload["doorOpen"] = doorOpen;
}

void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "door_sensor";
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
  StaticJsonDocument<128> doc;
  String msg = "";
  for (size_t i = 0; i < length; i++) msg += (char)payload[i];
  DeserializationError err = deserializeJson(doc, msg);
  if (err) return;
  // Handle ping message from server
  if (doc.containsKey("type") && String((const char*)doc["type"]) == "ping") {
    updateJSONData(digitalRead(BUTTON_PIN));
    sendWebSocketData();
  }
}

void setup() {
  Serial.begin(19200);
  
  MyWiFi::connect();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  initializeJSON();
  lastButtonState = digitalRead(BUTTON_PIN);
  updateJSONData(lastButtonState);
  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT);
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

void loop() {
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

  // Watchdog WebSocket
  if (!webSocketClient.isConnected()) {
    if (millis() - lastWsConnected > WS_RECONNECT_TIMEOUT && millis() - lastWsAttempt > 5000) {
      Serial.println("WebSocket nie odpowiada, restart połączenia...");
      webSocketClient.disconnect();
      delay(100);
      webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT);
      lastWsAttempt = millis();
    }
  }
}