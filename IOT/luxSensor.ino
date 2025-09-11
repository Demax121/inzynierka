#include <Wire.h>
#include <Adafruit_VEML7700.h>
#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

// Konfiguracja
#define SDA_PIN 4
#define SCL_PIN 5
#define WEBSOCKET_SERVER "192.168.1.4"
#define WEBSOCKET_PORT 8886

const unsigned int SENSOR_SEND_INTERVAL = 10000;
const unsigned int WEBSOCKET_RECONNECT_INTERVAL = 5000;
const int LUX_THRESHOLD = 10;

WebSocketsClient webSocket;
Adafruit_VEML7700 veml;
StaticJsonDocument<256> jsonPayload;

unsigned int lastSendMs = 0;
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

  webSocket.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT);
  webSocket.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
  webSocket.onEvent([](WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_CONNECTED) {
      identifyDevice();
    }
  });
}

void loop() {
  webSocket.loop();
  unsigned int now = millis();

  int currentLux = sensorReady ? (int)veml.readLux() : -1;

  if (abs(currentLux - lastLux) >= LUX_THRESHOLD || (now - lastSendMs >= SENSOR_SEND_INTERVAL)) {
    updateJSONData(currentLux);

    char buf[200];
    size_t n = serializeJson(jsonPayload, buf);
    webSocket.sendTXT(buf, n);
    lastLux = currentLux;
    lastSendMs = now;
  }
}
