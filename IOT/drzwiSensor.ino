#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

const char* WEBSOCKET_SERVER = "192.168.1.4";
const int WEBSOCKET_PORT = 8886;
const char* WIFI_AP_NAME = "Sensor Drzwi AP";
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;

const int BUTTON_PIN = 21;
int lastButtonState = HIGH;
const unsigned long DEBOUNCE_MS = 30;
const unsigned long HEARTBEAT_INTERVAL_MS = 10000;
unsigned long lastChangeTime = 0;
unsigned long lastHeartbeatMs = 0;

WebSocketsClient webSocketClient;
StaticJsonDocument<100> jsonPayload;

void initializeJSON() { jsonPayload["channel"] = "doorStatus"; jsonPayload["status"] = ""; }
void updateJSONData(int buttonState) { jsonPayload["status"] = (buttonState == LOW) ? "zamkniete" : "otwarte"; }
void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  char buf[64];
  size_t n = serializeJson(jsonPayload, buf, sizeof(buf));
  webSocketClient.sendTXT(buf, n);
}

void setup() {
  Serial.begin(19200);
  WiFiManager wm;
  wm.autoConnect(WIFI_AP_NAME);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  initializeJSON();
  lastButtonState = digitalRead(BUTTON_PIN);
  updateJSONData(lastButtonState);
  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"doorStatus\"}");
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
