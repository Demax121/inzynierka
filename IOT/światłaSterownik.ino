#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

WebSocketsClient webSocketClient;

const int RELAY_PIN = 27;
const int TOUCH_BUTTON_PIN = 22;
const int RELAY_ACTIVE_LEVEL = LOW;
const int RELAY_INACTIVE_LEVEL = HIGH;
const int BUTTON_ACTIVE_LEVEL = HIGH;
const unsigned long BUTTON_DEBOUNCE_MS = 25;
const unsigned long HEARTBEAT_INTERVAL_MS = 10000;

bool State = false;
int lastButtonState = LOW;
unsigned long lastButtonChangeMs = 0;
unsigned long lastHeartbeatMs = 0;

const char* WEBSOCKET_SERVER = "192.168.1.4";
const int WEBSOCKET_PORT = 8886;
const char* WIFI_AP_NAME = "Główne Światła AP";
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;


StaticJsonDocument<64> jsonPayload;

void initializeJSON() { jsonPayload["channel"] = "mainLights"; jsonPayload["lightStatus"] = ""; }
void updateJSONData(int relayState) { jsonPayload["lightStatus"] = (relayState == RELAY_ACTIVE_LEVEL) ? "ON" : "OFF"; }
void setRelay(bool on) { State = on; digitalWrite(RELAY_PIN, on ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL); updateJSONData(digitalRead(RELAY_PIN)); }

void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  char buf[48];
  size_t n = serializeJson(jsonPayload, buf, sizeof(buf));
  webSocketClient.sendTXT(buf, n);
}

void handleIncomingText(uint8_t* payload, size_t length) {
  if (!payload || length == 0) return;
  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) return;
  if (!doc.containsKey("lightStatus")) return;
  if (doc.containsKey("channel") && strcmp(doc["channel"], "mainLights") != 0) return;
  const char* cmd = doc["lightStatus"];
  if (!cmd) return;
  if (strcmp(cmd, "ON") == 0) {
    setRelay(true);
    sendWebSocketData();
  } else if (strcmp(cmd, "OFF") == 0) {
    setRelay(false);
    sendWebSocketData();
  }
}

void setup() {
  Serial.begin(19200);
  digitalWrite(RELAY_PIN, RELAY_INACTIVE_LEVEL);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TOUCH_BUTTON_PIN, INPUT);
  initializeJSON();
  setRelay(false);
  WiFiManager wm; wm.setDebugOutput(false); wm.autoConnect(WIFI_AP_NAME);
  uint8_t mac[6]; WiFi.macAddress(mac);
  Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"mainLights\"}");
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

  int rawButton = digitalRead(TOUCH_BUTTON_PIN);
  if (rawButton == BUTTON_ACTIVE_LEVEL && lastButtonState != BUTTON_ACTIVE_LEVEL) {
    unsigned long now = millis();
    if (now - lastButtonChangeMs >= BUTTON_DEBOUNCE_MS) {
      delay(BUTTON_DEBOUNCE_MS);
      if (digitalRead(TOUCH_BUTTON_PIN) == BUTTON_ACTIVE_LEVEL) {
        lastButtonChangeMs = now;
        setRelay(!State);
        sendWebSocketData();
      }
    }
  }
  lastButtonState = rawButton;

  unsigned long now = millis();
  if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    updateJSONData(digitalRead(RELAY_PIN));
    sendWebSocketData();
    lastHeartbeatMs = now;
  }
}