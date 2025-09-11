#include <Wire.h>
#include <Adafruit_VEML7700.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <MyWiFi.h>

#define SDA_PIN 4
#define SCL_PIN 5
#define WEBSOCKET_SERVER "192.168.1.4"
#define WEBSOCKET_PORT 8886

const unsigned int SENSOR_SEND_INTERVAL = 10000;
const unsigned int WEBSOCKET_RECONNECT_INTERVAL = 5000;
const int LUX_THRESHOLD = 10;

WebSocketsClient webSocket;
Adafruit_VEML7700 veml;
StaticJsonDocument<64> jsonPayload;

unsigned int lastSendMs = 0;
int lastLux = -999;
bool sensorReady = false;

void sendWebSocketData() {
  if (!webSocket.isConnected()) return;
  jsonPayload["lux"] = sensorReady ? (int)veml.readLux() : -1;
  char buf[64];
  size_t n = serializeJson(jsonPayload, buf, sizeof(buf));
  webSocket.sendTXT(buf, n);
  Serial.printf("[TX] Lux: %d\n", (int)jsonPayload["lux"]);
}

void handleIncomingText(uint8_t *payload, size_t length) {
  DeserializationError err = deserializeJson(jsonPayload, payload, length);
  if (err) return;

  if (jsonPayload.containsKey("type") && strcmp(jsonPayload["type"], "heartbeat") == 0) {
    jsonPayload["lux"] = sensorReady ? (int)veml.readLux() : -1;
    sendWebSocketData();
    Serial.println("[Heartbeat] Reply sent to server");
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

  jsonPayload["channel"] = "luxSensor";
  jsonPayload["lux"] = sensorReady ? (int)veml.readLux() : -1;
  lastLux = jsonPayload["lux"];

  webSocket.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT);
  webSocket.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
  webSocket.onEvent([](WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_CONNECTED) {
      webSocket.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"luxSensor\"}");
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
    }
  });
}

void loop() {
  webSocket.loop();
  unsigned int now = millis();

  int currentLux = sensorReady ? (int)veml.readLux() : -1;

  if (abs(currentLux - lastLux) >= LUX_THRESHOLD || (now - lastSendMs >= SENSOR_SEND_INTERVAL)) {
    jsonPayload["lux"] = currentLux;

    char buf[64];
    size_t n = serializeJson(jsonPayload, buf);
    webSocket.sendTXT(buf, n);
    lastLux = currentLux;
    lastSendMs = now;
  }
}
