#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

const char* WEBSOCKET_SERVER = "192.168.1.4";
const int WEBSOCKET_PORT = 8886;
const unsigned int WEBSOCKET_RECONNECT_INTERVAL = 5000;

#define SDA_PIN 4
#define SCL_PIN 5
const uint8_t BME280_I2C_ADDRESS = 0x76;
const unsigned int SENSOR_SEND_INTERVAL = 10000;

Adafruit_BME280 bme;
WebSocketsClient webSocketClient;
unsigned int lastSendTime = 0;
StaticJsonDocument<256> jsonPayload;

void initializeJSON() { 
  jsonPayload["identity"] = "room_stats_sensor";
  jsonPayload["channel"] = "room_stats";
  JsonObject payload = jsonPayload.createNestedObject("payload");
  payload["temperature"] = "";
  payload["humidity"] = "";
  payload["pressure"] = "";
}

void updateJSONData() {
  JsonObject payload = jsonPayload["payload"];
  payload["temperature"] = (int)round(bme.readTemperature());
  payload["humidity"] = (int)round(bme.readHumidity());
  payload["pressure"] = (int)(bme.readPressure() / 100);
}

void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "room_stats";
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
  Wire.begin(SDA_PIN, SCL_PIN);
  bme.begin(BME280_I2C_ADDRESS);
  initializeJSON();
  updateJSONData();
  
  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT);
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      identifyDevice();
      updateJSONData();
      sendWebSocketData();
      lastSendTime = millis();
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
}

void loop() {
  webSocketClient.loop();
  unsigned int now = millis();
  if (now - lastSendTime >= SENSOR_SEND_INTERVAL) {
    updateJSONData();
    sendWebSocketData();
    lastSendTime = now;
  }
}
