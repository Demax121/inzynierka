#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <MyWiFi.h>

#define BUTTON_PIN 23

WebSocketsClient webSocketClient;
StaticJsonDocument<128> jsonPayload;

const char* WEBSOCKET_SERVER = "192.168.1.4";
const int   WEBSOCKET_PORT   = 3000;
const unsigned long RECONNECT_INTERVAL = 5000;

void initializeJSON() {
  jsonPayload["channel"] = "doorStatus";
  jsonPayload["status"]  = digitalRead(BUTTON_PIN) == LOW ? "OPEN" : "CLOSED";
}

void updateJSONData(int pinValue) {
  jsonPayload["status"] = (pinValue == LOW) ? "OPEN" : "CLOSED";
}

void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  char buf[80];
  size_t n = serializeJson(jsonPayload, buf, sizeof(buf));
  webSocketClient.sendTXT(buf, n);
  Serial.printf("[TX] Door status: %s\n", (const char*)jsonPayload["status"]);
}

void handleIncomingText(uint8_t* payload, size_t length) {
  DeserializationError err = deserializeJson(jsonPayload, payload, length);
  if (err) return;

  if (jsonPayload.containsKey("type") && strcmp(jsonPayload["type"], "heartbeat") == 0) {
    updateJSONData(digitalRead(BUTTON_PIN));
    sendWebSocketData();
    Serial.println("[Heartbeat] Reply sent to server");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  MyWiFi::connect();

  initializeJSON();

  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      Serial.println("Połączono z WebSocket – identyfikacja jako doorStatus");
      webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"doorStatus\"}");
      sendWebSocketData();
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
    } else if (type == WStype_DISCONNECTED) {
      Serial.println("Rozłączono z WebSocket");
    }
  });
  webSocketClient.setReconnectInterval(RECONNECT_INTERVAL);
}

void loop() {
  webSocketClient.loop();

  static int lastState = HIGH;
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastState) {
    delay(50);
    updateJSONData(reading);
    sendWebSocketData();
    lastState = reading;
  }
}
