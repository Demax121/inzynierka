#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <MyWiFi.h>

#define RELAY_PIN  26

WebSocketsClient webSocketClient;
StaticJsonDocument<128> jsonPayload;

const char* WEBSOCKET_SERVER = "192.168.1.4";
const int   WEBSOCKET_PORT   = 3000;
const unsigned long RECONNECT_INTERVAL = 5000;

bool lightOn = false;


void initializeJSON() {
  jsonPayload["channel"] = "mainLights";
  jsonPayload["lightStatus"] = lightOn ? "ON" : "OFF";
}

void updateJSONData() {
  jsonPayload["lightStatus"] = lightOn ? "ON" : "OFF";
}

void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  char buf[80];
  size_t n = serializeJson(jsonPayload, buf, sizeof(buf));
  webSocketClient.sendTXT(buf, n);
  Serial.printf("[TX] Light status: %s\n", lightOn ? "ON" : "OFF");
}

void handleIncomingText(uint8_t* payload, size_t length) {
  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) return;


  if (doc.containsKey("type") && strcmp(doc["type"], "heartbeat") == 0) {
    updateJSONData();
    sendWebSocketData();
    Serial.println("[Heartbeat] Reply sent to server");
    return;
  }


  if (doc.containsKey("channel") && strcmp(doc["channel"], "mainLights") == 0) {
    if (doc.containsKey("lightStatus")) {
      const char* st = doc["lightStatus"];
      if (strcmp(st, "ON") == 0) {
        lightOn = true;
        digitalWrite(RELAY_PIN, HIGH);
      } else {
        lightOn = false;
        digitalWrite(RELAY_PIN, LOW);
      }
      updateJSONData();
      sendWebSocketData();
      Serial.printf("[RX] Light set to: %s\n", lightOn ? "ON" : "OFF");
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);


  MyWiFi::connect(); 


  initializeJSON();

  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      Serial.println("Połączono z WebSocket – identyfikacja jako mainLights");
      webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"mainLights\"}");
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
}
