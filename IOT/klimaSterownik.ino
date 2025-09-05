#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

WebSocketsClient webSocketClient; // spójnie z innymi plikami

// Konfiguracja połączenia
const char* WEBSOCKET_SERVER = "192.168.1.4"; // adres serwera
const int   WEBSOCKET_PORT   = 8886;            // port WS
const char* WIFI_AP_NAME     = "Klimatyzacja AP"; // nazwa konfiguracji WiFiManagera
const unsigned long RECONNECT_INTERVAL = 5000; // ms

// Przycisk
const int BUTTON_PIN = 20;            // dopasuj jeśli inny pin
const int BUTTON_ACTIVE_LEVEL = HIGH; // dla sensora dotykowego / pull-down
const unsigned long BUTTON_DEBOUNCE_MS = 25;

// Stan klimatyzacji
bool klimaOn = false; // aktualny stan (logiczny)
int lastButtonState = LOW;
unsigned long lastButtonChangeMs = 0;

// Dodane zmienne do kontroli temperatury
float currentTemp = 0.0;    // aktualna temperatura z czujnika
float requestedTemp = 25.0; // docelowa temperatura
const float HISTERAZA = 2.0; // histereza 2 stopnie

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
  Serial.printf("[TX] AC %s\n", klimaOn ? "Włączone" : "Wyłączone");
}

void checkTemperatureControl() {
  if (currentTemp == 0.0) return; // brak danych o temperaturze
  
  float tempDiff = currentTemp - requestedTemp;
  
  if (tempDiff > HISTERAZA) {
    // Za gorąco - włączamy chłodzenie
    if (!klimaOn) {
      klimaOn = true;
      Serial.println("CHŁODZIMY!!!");
      sendWebSocketData();
    }
  } else if (tempDiff < -HISTERAZA) {
    // Za zimno - włączamy grzanie
    if (!klimaOn) {
      klimaOn = true;
      Serial.println("GRZEJMY!!!");
      sendWebSocketData();
    }
  } else {
    // Temperatura w zakresie - wyłączamy klimę
    if (klimaOn) {
      klimaOn = false;
      Serial.println("Temperatura OK - wyłączam klimę");
      sendWebSocketData();
    }
  }
}

void handleIncomingText(uint8_t* payload, size_t length) {
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) return;
  const char* channel = doc["channel"] | "";
  if (strcmp(channel, "roomStats") == 0) {
    if (doc.containsKey("temperature")) {
      float t = doc["temperature"].as<float>();
      float h = doc["humidity"].as<float>();
      float p = doc["pressure"].as<float>();
      currentTemp = t; // zapisz aktualną temperaturę
      Serial.printf("[roomStats] T: %.2f°C  H: %.2f%%  P: %.2f hPa\n", t, h, p);
      checkTemperatureControl(); // sprawdź czy trzeba włączyć/wyłączyć klimę
    }
  } else if (strcmp(channel, "klimatyzacja") == 0) {
    // Odczyt temperatury jeśli przyszła razem z kanałem klimatyzacji
    if (doc.containsKey("temperature")) {
      float t = doc["temperature"].as<float>();
      currentTemp = t; // zapisz aktualną temperaturę
      Serial.printf("[klimatyzacja] Ambient temperature: %.2f°C\n", t);
      checkTemperatureControl(); // sprawdź kontrolę temperatury
    }
    // Nowa sekcja dla requestedTemp
    if (doc.containsKey("requestedTemp")) {
      int reqTemp = doc["requestedTemp"].as<int>();
      requestedTemp = (float)reqTemp; // zapisz docelową temperaturę
      Serial.printf("[klimatyzacja] Requested temperature: %d°C\n", reqTemp);
      checkTemperatureControl(); // sprawdź kontrolę temperatury po zmianie docelowej
    }
    if (doc.containsKey("klimaStatus")) {
      const char* st = doc["klimaStatus"];
      if (st && (strcmp(st, "ON") == 0 || strcmp(st, "OFF") == 0)) {
        bool newState = (strcmp(st, "ON") == 0);
        if (newState != klimaOn) {
          klimaOn = newState;
          Serial.printf("[RX] AC %s (z sieci)\n", klimaOn ? "Włączone" : "Wyłączone");
        } else {
          Serial.printf("[RX] AC %s (bez zmiany)\n", klimaOn ? "Włączone" : "Wyłączone");
        }
      }
    }
  }
}

void setup() {
  Serial.begin(19200);
  pinMode(BUTTON_PIN, INPUT);
  WiFiManager wm; wm.setDebugOutput(false); wm.autoConnect(WIFI_AP_NAME);
  uint8_t mac[6]; WiFi.macAddress(mac);
  Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

  initializeJSON();

  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      Serial.println("Połączono z WS – identyfikacja kanału klimatyzacja");
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
        Serial.printf("[BUTTON] AC %s\n", klimaOn ? "Włączone" : "Wyłączone");
        sendWebSocketData();
      }
    }
  }
  lastButtonState = raw;
}