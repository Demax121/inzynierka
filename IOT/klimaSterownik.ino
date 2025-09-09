#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

// Piny dla ESP32 (wyświetlacz)
#define TFT_CS   5
#define TFT_DC   21
#define TFT_RST  22

// Tworzymy obiekt ILI9341
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

WebSocketsClient webSocketClient;

// Konfiguracja połączenia
const char* WEBSOCKET_SERVER = "192.168.1.4"; // adres serwera
const int   WEBSOCKET_PORT   = 8886;            // port WS
const char* WIFI_AP_NAME     = "Klimatyzacja AP"; // nazwa konfiguracji WiFiManagera
const unsigned long RECONNECT_INTERVAL = 5000; // ms

// Stan klimatyzacji
bool klimaOn = false; // aktualny stan (logiczny)

// Dodane zmienne do kontroli temperatury
float currentTemp = 0.0;    // aktualna temperatura z czujnika
float requestedTemp = 25.0; // docelowa temperatura
const float HISTERAZA = 2.0; // histereza 2 stopnie
String currentFunction = ""; // aktualnie wykonywana funkcja

// Bufor roboczy JSON (wejściowy) + payload wyjściowy
StaticJsonDocument<256> doc;
StaticJsonDocument<64> jsonPayload; // analogicznie do innych sterowników

void initializeDisplay() {
  tft.begin();                 // Inicjalizacja ILI9341
  tft.setRotation(1);          // Obrót ekranu (0-3)
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  
  // Aktualna temperatura na górze
  tft.setCursor(10, 20);
  tft.println("TEMP AKTUALNA:");
  
  // Odstęp i oczekiwana temperatura
  tft.setCursor(10, 100);
  tft.println("TEMP OCZEKIWANA:");
  
  // Większy odstęp i status + funkcja obok siebie
  tft.setCursor(10, 180);
  tft.println("STATUS:");
  
  tft.setCursor(160, 180);
  tft.println("FUNKCJA:");
}

void updateDisplay() {
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2); // Zmniejszony tekst dla temperatur
  
  // Wyczyść i wyświetl aktualną temperaturę
  tft.fillRect(10, 50, 300, 30, ILI9341_BLACK);
  tft.setCursor(10, 50);
  if (currentTemp > 0.0) {
    tft.print(currentTemp, 1);
    tft.print(" C");
  } else {
    tft.print("-- C");
  }
  
  // Wyczyść i wyświetl oczekiwaną temperaturę
  tft.fillRect(10, 130, 300, 30, ILI9341_BLACK);
  tft.setCursor(10, 130);
  tft.print(requestedTemp, 0);
  tft.print(" C");
  
  // Wyczyść i wyświetl status klimatyzacji (lewa strona)
  tft.fillRect(10, 210, 140, 25, ILI9341_BLACK);
  tft.setCursor(10, 210);
  tft.setTextSize(2);
  if (klimaOn) {
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.print("ON");
  } else {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.print("OFF");
  }
  
  // Wyczyść i wyświetl funkcję (prawa strona)
  tft.fillRect(160, 210, 150, 25, ILI9341_BLACK);
  tft.setCursor(160, 210);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setTextSize(2); // Zwiększony tekst dla funkcji
  if (currentFunction.length() > 0) {
    tft.print(currentFunction);
  } else {
    tft.print("GOTOWY");
  }
}

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
  bool previousState = klimaOn;
  
  if (tempDiff > HISTERAZA) {
    // Za gorąco - włączamy chłodzenie
    if (!klimaOn) {
      klimaOn = true;
      currentFunction = "CHLODZIMY!!!";
      Serial.println("CHŁODZIMY!!!");
      sendWebSocketData();
      updateDisplay();
    }
  } else if (tempDiff < -HISTERAZA) {
    // Za zimno - włączamy grzanie
    if (!klimaOn) {
      klimaOn = true;
      currentFunction = "GRZEJEMY!!!";
      Serial.println("GRZEJMY!!!");
      sendWebSocketData();
      updateDisplay();
    }
  } else {
    // Temperatura w zakresie - wyłączamy klimę
    if (klimaOn) {
      klimaOn = false;
      currentFunction = "";
      Serial.println("Temperatura OK - wyłączam klimę");
      sendWebSocketData();
      updateDisplay();
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
      updateDisplay(); // aktualizuj wyświetlacz po otrzymaniu nowej temperatury
      checkTemperatureControl(); // sprawdź czy trzeba włączyć/wyłączyć klimę
    }
  } else if (strcmp(channel, "klimatyzacja") == 0) {
    // Odczyt temperatury jeśli przyszła razem z kanałem klimatyzacji
    if (doc.containsKey("temperature")) {
      float t = doc["temperature"].as<float>();
      currentTemp = t; // zapisz aktualną temperaturę
      Serial.printf("[klimatyzacja] Ambient temperature: %.2f°C\n", t);
      updateDisplay(); // aktualizuj wyświetlacz
      checkTemperatureControl(); // sprawdź kontrolę temperatury
    }
    // Nowa sekcja dla requestedTemp
    if (doc.containsKey("requestedTemp")) {
      int reqTemp = doc["requestedTemp"].as<int>();
      requestedTemp = (float)reqTemp; // zapisz docelową temperaturę
      Serial.printf("[klimatyzacja] Requested temperature: %d°C\n", reqTemp);
      updateDisplay(); // aktualizuj wyświetlacz po zmianie docelowej temperatury
      checkTemperatureControl(); // sprawdź kontrolę temperatury po zmianie docelowej
    }
    if (doc.containsKey("klimaStatus")) {
      const char* st = doc["klimaStatus"];
      if (st && (strcmp(st, "ON") == 0 || strcmp(st, "OFF") == 0)) {
        bool newState = (strcmp(st, "ON") == 0);
        if (newState != klimaOn) {
          klimaOn = newState;
          updateDisplay(); // aktualizuj wyświetlacz po zmianie stanu
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
  delay(500);

  // Inicjalizacja wyświetlacza
  initializeDisplay();

  WiFiManager wm; wm.setDebugOutput(false); wm.autoConnect(WIFI_AP_NAME);

  initializeJSON();

  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      Serial.println("Połączono z WS – identyfikacja kanału klimatyzacja");
      webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"klimatyzacja\"}");
      sendWebSocketData();
      updateDisplay(); // aktualizuj wyświetlacz po połączeniu
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
    }
  });
  webSocketClient.setReconnectInterval(RECONNECT_INTERVAL);
}

void loop() {
  webSocketClient.loop();
}