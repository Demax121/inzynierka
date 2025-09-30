#include <Wire.h>
#include <Adafruit_VEML7700.h>
#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

// Konfiguracja
#define SDA_PIN 21
#define SCL_PIN 22

String WEBSOCKET_SERVER = "192.168.1.2";
const int WEBSOCKET_PORT = 8886;


const unsigned int WEBSOCKET_RECONNECT_INTERVAL = 5000;
const int LUX_THRESHOLD = 10;

WebSocketsClient webSocket;
Adafruit_VEML7700 veml;
StaticJsonDocument<256> jsonPayload;

// Watchdog WebSocket
unsigned long lastWsConnected = 0;
unsigned long lastWsAttempt = 0;
const unsigned long WS_RECONNECT_TIMEOUT = 15000;


int lastLux = -999;
bool sensorReady = false;

void initializeJSON() {
  jsonPayload["identity"] = "lux_sensor";
  jsonPayload["channel"] = "lux_sensor";
  JsonObject payload = jsonPayload.createNestedObject("payload");
  payload["lux"] = -1;
}

void updateJSONData(int currentLux) {
  JsonObject payload = jsonPayload["payload"];
  payload["lux"] = currentLux;
}

void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "lux_sensor";
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocket.sendTXT(idMessage);
}

void sendWebSocketData() {
  if (!webSocket.isConnected()) return;
  String jsonStr;
  serializeJson(jsonPayload, jsonStr);
  webSocket.sendTXT(jsonStr);
}

void handleWebSocketMessage(uint8_t* payload, size_t length) {
  StaticJsonDocument<128> doc;
  String msg = "";
  for (size_t i = 0; i < length; i++) msg += (char)payload[i];
  DeserializationError err = deserializeJson(doc, msg);
  if (err) return;
  // Handle ping message from server
  if (doc.containsKey("type") && String((const char*)doc["type"]) == "ping") {
    int currentLux = sensorReady ? (int)veml.readLux() : -1;
    updateJSONData(currentLux);
    sendWebSocketData();
  }
}

void setup() {
  Serial.begin(19200);
  MyWiFi::connect();



  Wire.begin(SDA_PIN, SCL_PIN);

  sensorReady = veml.begin();
  if (sensorReady) {
    veml.setGain(VEML7700_GAIN_1);
    veml.setIntegrationTime(VEML7700_IT_100MS);
    veml.powerSaveEnable(false);
  }

  initializeJSON();
  int initialLux = sensorReady ? (int)veml.readLux() : -1;
  updateJSONData(initialLux);
  lastLux = initialLux;

  webSocket.begin(WEBSOCKET_SERVER.c_str(), WEBSOCKET_PORT);
  webSocket.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
  webSocket.onEvent([](WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_CONNECTED) {
      identifyDevice();
    } else if (type == WStype_TEXT) {
      handleWebSocketMessage(payload, length);
    }
  });

    lastWsConnected = millis();
  lastWsAttempt = millis();
}

void loop() {
  webSocket.loop();

  int currentLux = sensorReady ? (int)veml.readLux() : -1;

  if (abs(currentLux - lastLux) >= LUX_THRESHOLD) {
    updateJSONData(currentLux);
    sendWebSocketData();
    lastLux = currentLux;
  }

    // Watchdog WebSocket
  if (!webSocket.isConnected()) {
    if (millis() - lastWsConnected > WS_RECONNECT_TIMEOUT && millis() - lastWsAttempt > 5000) {
      Serial.println("WebSocket nie odpowiada, restart połączenia...");
      webSocket.disconnect();
      delay(100);
  webSocket.begin(WEBSOCKET_SERVER.c_str(), WEBSOCKET_PORT);
      lastWsAttempt = millis();
    }
  }
}
