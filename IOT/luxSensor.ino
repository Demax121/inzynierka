#include <Wire.h>
#include <Adafruit_VEML7700.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

// Konfiguracja
#define SDA_PIN 4
#define SCL_PIN 5
#define WIFI_AP_NAME "Lux Sensor AP"
#define WEBSOCKET_SERVER "192.168.1.4"
#define WEBSOCKET_PORT 8886

const unsigned long SENSOR_SEND_INTERVAL = 10000;
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;
const float LUX_THRESHOLD = 10; // próg zmiany w lux do wysłania natychmiast

WebSocketsClient webSocket;
Adafruit_VEML7700 veml;
StaticJsonDocument<64> jsonPayload;

unsigned long lastSendMs = 0;
float lastLux = -999;
bool sensorReady = false;

void setup() {
  Serial.begin(19200);
  WiFiManager wm;
  wm.setDebugOutput(false);
  wm.autoConnect(WIFI_AP_NAME);

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
      Serial.println("WebSocket połączony.");
      webSocket.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"luxSensor\"}");
    }
  });
}

void loop() {
  webSocket.loop();
  unsigned long now = millis();

  float currentLux = sensorReady ? veml.readLux() : -1;

  if (fabs(currentLux - lastLux) >= LUX_THRESHOLD || (now - lastSendMs >= SENSOR_SEND_INTERVAL)) {
    jsonPayload["lux"] = (int)currentLux;

    char buf[64];
    size_t n = serializeJson(jsonPayload, buf);
    webSocket.sendTXT(buf, n);
    Serial.printf("Wysłano: %s\n", buf);
    lastLux = currentLux;
    lastSendMs = now;
  }
}
