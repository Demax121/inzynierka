#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <MyWiFi.h>



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
StaticJsonDocument<200> jsonPayload;

void initializeJSON() { jsonPayload["channel"] = "roomStats"; jsonPayload["temperature"] = ""; jsonPayload["humidity"] = ""; jsonPayload["pressure"] = ""; }
void updateJSONData() {
  jsonPayload["temperature"] = (int)round(bme.readTemperature());
  jsonPayload["humidity"] = (int)round(bme.readHumidity());
  jsonPayload["pressure"] = (int)(bme.readPressure() / 100);
}
void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  char buf[160];
  size_t n = serializeJson(jsonPayload, buf, sizeof(buf));
  webSocketClient.sendTXT(buf, n);
}

void handleIncomingText(uint8_t* payload, size_t length) {
  DeserializationError err = deserializeJson(jsonPayload, payload, length);
  if (err) return;

  if (jsonPayload.containsKey("type") && strcmp(jsonPayload["type"], "heartbeat") == 0) {
    updateJSONData();
    sendWebSocketData();
    Serial.println("[Heartbeat] Reply sent to server");
  }
}

void setup() {
  Serial.begin(19200);
  MyWiFi::connect();

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
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
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
