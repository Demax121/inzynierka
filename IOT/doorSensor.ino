#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

const char* WEBSOCKET_SERVER = "192.168.1.4";
const int WEBSOCKET_PORT = 8886;
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;

const int BUTTON_PIN = 21;
int lastButtonState = HIGH;
const unsigned long DEBOUNCE_MS = 30;
const unsigned long HEARTBEAT_INTERVAL_MS = 10000;
unsigned long lastChangeTime = 0;
unsigned long lastHeartbeatMs = 0;

WebSocketsClient webSocketClient;
StaticJsonDocument<256> jsonPayload;
bool doorOpen = false;

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
  char buf[200];
  size_t n = serializeJson(jsonPayload, buf, sizeof(buf));
  webSocketClient.sendTXT(buf, n);
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
      identifyDevice();
      int currentButtonState = digitalRead(BUTTON_PIN);
      updateJSONData(currentButtonState);
      sendWebSocketData();
      lastButtonState = currentButtonState;
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
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
  unsigned long now = millis();
  if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    updateJSONData(digitalRead(BUTTON_PIN));
    sendWebSocketData();
    lastHeartbeatMs = now;
  }
}
