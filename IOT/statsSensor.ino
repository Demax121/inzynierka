#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

const char* WEBSOCKET_SERVER = "192.168.1.4";
const int WEBSOCKET_PORT = 8886;
const char* WIFI_AP_NAME = "Statystyki Pomieszczenia AP";
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;

#define SDA_PIN 4
#define SCL_PIN 5
const uint8_t BME280_I2C_ADDRESS = 0x76;
const unsigned long SENSOR_SEND_INTERVAL = 10000;

Adafruit_BME280 bme;
WebSocketsClient webSocketClient;
unsigned long lastSendTime = 0;
StaticJsonDocument<200> jsonPayload;

void initializeJSON() { jsonPayload["channel"] = "roomStats"; jsonPayload["temperature"] = ""; jsonPayload["humidity"] = ""; jsonPayload["pressure"] = ""; }
void updateJSONData() {
  jsonPayload["temperature"] = (int)round(bme.readTemperature());
  jsonPayload["humidity"] = (int)round(bme.readHumidity());
  jsonPayload["pressure"] = (int)round(bme.readPressure() / 100.0F);
}
void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  char buf[160];
  size_t n = serializeJson(jsonPayload, buf, sizeof(buf));
  webSocketClient.sendTXT(buf, n);
}

void setup() {
  Serial.begin(19200);
  WiFiManager wm; wm.autoConnect(WIFI_AP_NAME);
  Wire.begin(SDA_PIN, SCL_PIN);
  bme.begin(BME280_I2C_ADDRESS);
  initializeJSON();
  updateJSONData();
  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"roomStats\"}");
      updateJSONData();
      sendWebSocketData();
      lastSendTime = millis();
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
}

void loop() {
  webSocketClient.loop();
  if (millis() - lastSendTime >= SENSOR_SEND_INTERVAL) {
    updateJSONData();
    sendWebSocketData();
    lastSendTime = millis();
  }
}
