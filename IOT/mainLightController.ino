#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

WebSocketsClient webSocketClient;

const int RELAY_PIN = 27;
const int TOUCH_BUTTON_PIN = 22;
const int RELAY_ACTIVE_LEVEL = LOW;
const int RELAY_INACTIVE_LEVEL = HIGH;
const int BUTTON_ACTIVE_LEVEL = HIGH;
const unsigned int BUTTON_DEBOUNCE_MS = 50;
const unsigned int HEARTBEAT_INTERVAL_MS = 10000;

bool State = false;
int lastButtonState = LOW;
int buttonState = LOW;
unsigned int lastButtonChangeMs = 0;
unsigned int lastHeartbeatMs = 0;

const char* WEBSOCKET_SERVER = "192.168.1.4";
const int WEBSOCKET_PORT = 8886;
const unsigned int WEBSOCKET_RECONNECT_INTERVAL = 5000;

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
  char buf[200];
  size_t n = serializeJson(jsonPayload, buf, sizeof(buf));
  webSocketClient.sendTXT(buf, n);
}

void handleIncomingText(uint8_t* payload, size_t length) {
  if (!payload || length == 0) return;
  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) return;
  if (!doc.containsKey("lightON")) return;
  if (doc.containsKey("channel") && strcmp(doc["channel"], "main_lights") != 0) return;
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
  MyWiFi::connect();
  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      identifyDevice();
      sendWebSocketData();
      lastHeartbeatMs = millis();
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
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

  // Heartbeat
  unsigned int now = millis();
  if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    updateJSONData(State);
    sendWebSocketData();
    lastHeartbeatMs = now;
  }
}