#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <AESCrypto.h>

String WEBSOCKET_SERVER = "192.168.1.2";
const int WEBSOCKET_PORT = 8884;
const unsigned int WEBSOCKET_RECONNECT_INTERVAL = 5000;
String device_api_key = "b3odiEjCNSf7";
String encryption_key = "xuCmb33pFJgJAwR5";

#define SDA_PIN 4
#define SCL_PIN 5
const uint8_t BME280_I2C_ADDRESS = 0x76;
const int TEMPERATURE_THRESHOLD = 2;  // 2 stopnie progu zmiany temperatury (jako int)

AESCrypto crypto(encryption_key);

// Watchdog WebSocket
unsigned long lastWsConnected = 0;
unsigned long lastWsAttempt = 0;
const unsigned long WS_RECONNECT_TIMEOUT = 15000;

Adafruit_BME280 bme;
WebSocketsClient webSocketClient;
StaticJsonDocument<256> jsonPayload; // not used for payload body anymore

// Tylko jedna zmienna do śledzenia ostatniej temperatury jako float
float lastTemperature = -100.0;  // Inicjalizacja wartością niemożliwą

void initializeJSON() { 
  // envelope created on send
}

void updateJSONData() {
  JsonObject payload = jsonPayload["payload"];
  payload["temperature"] = (float)bme.readTemperature();
  payload["humidity"] = (float)bme.readHumidity();
  payload["pressure"] = (float)(bme.readPressure() / 100.0);
}

void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "room_stats";
  idDoc["device_api_key"] = device_api_key;
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocketClient.sendTXT(idMessage);
}

void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  StaticJsonDocument<192> p;
  p["temperature"] = (float)bme.readTemperature();
  p["humidity"] = (float)bme.readHumidity();
  p["pressure"] = (float)(bme.readPressure() / 100.0);
  String plain; serializeJson(p, plain);
  String iv = AESCrypto::generateIV();
  String cipher = crypto.encrypt(plain, iv);
  StaticJsonDocument<256> env;
  env["identity"] = "room_stats_sensor";
  env["channel"] = "room_stats";
  env["device_api_key"] = device_api_key;
  env["msgIV"] = iv;
  env["payload"] = cipher;
  String out; serializeJson(env, out);
  webSocketClient.sendTXT(out);
}

void setup() {
  Serial.begin(19200);
  MyWiFi::connect();
  Wire.begin(SDA_PIN, SCL_PIN);
  bme.begin(BME280_I2C_ADDRESS);
  initializeJSON();
  
  // Pobierz początkową temperaturę w formacie int
  lastTemperature = (float)bme.readTemperature();
  updateJSONData();
  
  webSocketClient.begin(WEBSOCKET_SERVER.c_str(), WEBSOCKET_PORT);
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      lastWsConnected = millis(); // aktualizuj czas połączenia
      identifyDevice();
      updateJSONData();
      sendWebSocketData();
    }
    else if (type == WStype_TEXT) {
      // Handle ping from server
      StaticJsonDocument<128> doc;
      String msg = "";
      for (size_t i = 0; i < length; i++) msg += (char)payload[i];
      DeserializationError err = deserializeJson(doc, msg);
      if (!err && doc.containsKey("type") && String((const char*)doc["type"]) == "ping") {
        updateJSONData();
        sendWebSocketData();
      }
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
    lastWsConnected = millis();
  lastWsAttempt = millis();
}

void loop() {
  webSocketClient.loop();
  
  // Odczytaj aktualną temperaturę bezpośrednio jako int
  float currentTemperature = (float)bme.readTemperature();
  
  // Sprawdź, czy temperatura zmieniła się o więcej niż próg
  if (fabs(currentTemperature - lastTemperature) >= TEMPERATURE_THRESHOLD) {
    updateJSONData();
    sendWebSocketData();
    lastTemperature = currentTemperature;
  }

    // Watchdog WebSocket
  if (!webSocketClient.isConnected()) {
    if (millis() - lastWsConnected > WS_RECONNECT_TIMEOUT && millis() - lastWsAttempt > 5000) {
      Serial.println("WebSocket nie odpowiada, restart połączenia...");
      webSocketClient.disconnect();
      delay(100);
  webSocketClient.begin(WEBSOCKET_SERVER.c_str(), WEBSOCKET_PORT);
      lastWsAttempt = millis();
    }
  }
}
