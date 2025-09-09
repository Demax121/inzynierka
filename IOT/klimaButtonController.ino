#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>


WebSocketsClient webSocketClient;

// Konfiguracja połączenia
const char* WEBSOCKET_SERVER = "192.168.1.4"; // adres serwera
const int   WEBSOCKET_PORT   = 8886;            // port WS
const char* WIFI_AP_NAME     = "Klimatyzacja Button AP"; // nazwa konfiguracji WiFiManagera
const unsigned long RECONNECT_INTERVAL = 5000; // ms

// Przycisk
const int BUTTON_PIN = 20;            // dopasuj jeśli inny pin
const int BUTTON_ACTIVE_LEVEL = HIGH; // dla sensora dotykowego / pull-down
const unsigned long BUTTON_DEBOUNCE_MS = 25;

// Stan klimatyzacji
bool klimaOn = false; // aktualny stan (logiczny)
int lastButtonState = LOW;
unsigned long lastButtonChangeMs = 0;


// Bufor roboczy JSON (wejściowy) + payload wyjściowy
StaticJsonDocument<256> doc;
StaticJsonDocument<64> jsonPayload; // analogicznie do innych sterowników

void initializeJSON() {
  jsonPayload["channel"] = "klimatyzacja";
  jsonPayload["klimaStatus"] = klimaOn ? "ON" : "OFF";
}

void updateJSONData() {
  jsonPayload["klimaStatus"] = klimaOn ? "ON" : "OFF";
}

void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  updateJSONData();
  char buf[80];
  size_t n = serializeJson(jsonPayload, buf, sizeof(buf));
  webSocketClient.sendTXT(buf, n);
  // only print ON/OFF
  Serial.println(klimaOn ? "ON" : "OFF");
}


void handleIncomingText(uint8_t* payload, size_t length) {
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) return;
  const char* channel = doc["channel"] | "";
  if (strcmp(channel, "roomStats") == 0) {

  } if (strcmp(channel, "klimatyzacja") == 0) {
    // Only process klimaStatus, ignore temperature-related data
    if (doc.containsKey("klimaStatus")) {
      const char* st = doc["klimaStatus"];
      if (st && (strcmp(st, "ON") == 0 || strcmp(st, "OFF") == 0)) {
        bool newState = (strcmp(st, "ON") == 0);
        if (newState != klimaOn) {
          klimaOn = newState;
          // only print ON/OFF when state changes
          Serial.println(klimaOn ? "ON" : "OFF");
        }
      }
    }
  }
}

void setup() {
  Serial.begin(19200);

  
  pinMode(BUTTON_PIN, INPUT);
  WiFiManager wm; wm.setDebugOutput(false); wm.autoConnect(WIFI_AP_NAME);
  initializeJSON();

  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      // no serial print here, only identify channel
      webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"klimatyzacja\"}");
      sendWebSocketData();
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
    }
  });
  webSocketClient.setReconnectInterval(RECONNECT_INTERVAL);

}
void loop() {
  webSocketClient.loop();

  // Debounce & odczyt przycisku
  int raw = digitalRead(BUTTON_PIN);
  if (raw == BUTTON_ACTIVE_LEVEL && lastButtonState != BUTTON_ACTIVE_LEVEL) {
    unsigned long now = millis();
    if (now - lastButtonChangeMs >= BUTTON_DEBOUNCE_MS) {
      delay(BUTTON_DEBOUNCE_MS);
      if (digitalRead(BUTTON_PIN) == BUTTON_ACTIVE_LEVEL) {
        lastButtonChangeMs = now;
        klimaOn = !klimaOn;
        // only print ON/OFF
        Serial.println(klimaOn ? "ON" : "OFF");
        sendWebSocketData();
      }
    }
  }
  lastButtonState = raw;

}