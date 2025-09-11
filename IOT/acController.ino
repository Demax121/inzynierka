#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

#define TFT_CS   5
#define TFT_DC   21
#define TFT_RST  22

const int buttonPin = 13;
bool BTNstate = false;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

WebSocketsClient webSocketClient;

const char* WEBSOCKET_SERVER = "192.168.1.4";
const int   WEBSOCKET_PORT   = 8886;
const unsigned long RECONNECT_INTERVAL = 5000;

bool klimaOn = false;
bool manualOverride = false;

int currentTemp = 0;
int requestedTemp = 25;
const int HISTERAZA = 2;
char currentFunction[16] = "";

StaticJsonDocument<256> doc;
StaticJsonDocument<64> jsonPayload;

const char PROGMEM temp_aktualna[] = "TEMP AKTUALNA:";
const char PROGMEM temp_oczekiwana[] = "TEMP OCZEKIWANA:";
const char PROGMEM status_text[] = "STATUS:";
const char PROGMEM funkcja_text[] = "FUNKCJA:";
const char PROGMEM on_text[] = "ON";
const char PROGMEM off_text[] = "OFF";
const char PROGMEM spoczynku_text[] = "W SPOCYNKU";
const char PROGMEM space_c[] = " C";
const char PROGMEM dash_c[] = "-- C";
const char PROGMEM chlodzenie_text[] = "CHLODZIMY!!!";
const char PROGMEM grzanie_text[] = "GRZEJEMY!!!";

void initializeDisplay() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  
  tft.setCursor(10, 20);
  tft.print((__FlashStringHelper*)temp_aktualna);
  
  tft.setCursor(10, 100);
  tft.print((__FlashStringHelper*)temp_oczekiwana);
  
  tft.setCursor(10, 180);
  tft.print((__FlashStringHelper*)status_text);
  
  tft.setCursor(160, 180);
  tft.print((__FlashStringHelper*)funkcja_text);
}

void updateDisplay() {
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);
  
  tft.fillRect(10, 50, 300, 30, ILI9341_BLACK);
  tft.setCursor(10, 50);
  if (currentTemp > 0) {
    tft.print(currentTemp);
    tft.print((__FlashStringHelper*)space_c);
  } else {
    tft.print((__FlashStringHelper*)dash_c);
  }
  
  tft.fillRect(10, 130, 300, 30, ILI9341_BLACK);
  tft.setCursor(10, 130);
  tft.print(requestedTemp);
  tft.print((__FlashStringHelper*)space_c);
  
  tft.fillRect(10, 210, 140, 25, ILI9341_BLACK);
  tft.setCursor(10, 210);
  tft.setTextSize(2);
  if (klimaOn) {
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.print((__FlashStringHelper*)on_text);
  } else {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.print((__FlashStringHelper*)off_text);
  }
  
  tft.fillRect(160, 210, 150, 25, ILI9341_BLACK);
  tft.setCursor(160, 210);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setTextSize(2);
  if (strlen(currentFunction) > 0) {
    tft.print(currentFunction);
  } else {
    tft.print((__FlashStringHelper*)spoczynku_text);
  }
}

void initializeJSON() {
  jsonPayload["channel"] = "air_conditioning";
  jsonPayload["klimaStatus"] = klimaOn ? "ON" : "OFF";
  jsonPayload["currentFunction"] = currentFunction;
}

void updateJSONData() {
  jsonPayload["klimaStatus"] = klimaOn ? "ON" : "OFF";
  jsonPayload["currentFunction"] = currentFunction;
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
  if (currentTemp == 0) return;
  if (manualOverride) return;
  
  int tempDiff = currentTemp - requestedTemp;
  bool previousState = klimaOn;
  char previousFunction[16];
  strcpy(previousFunction, currentFunction);
  
  if (tempDiff > HISTERAZA) {
    klimaOn = true;
    strcpy_P(currentFunction, chlodzenie_text);
    if (!previousState || strcmp(previousFunction, currentFunction) != 0) {
      Serial.println("CHŁODZIMY!!!");
      sendWebSocketData();
      updateDisplay();
    }
  } else if (tempDiff < -HISTERAZA) {
    klimaOn = true;
    strcpy_P(currentFunction, grzanie_text);
    if (!previousState || strcmp(previousFunction, currentFunction) != 0) {
      Serial.println("GRZEJMY!!!");
      sendWebSocketData();
      updateDisplay();
    }
  } else {
    if (klimaOn) {
      klimaOn = false;
      strcpy(currentFunction, "");
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
  if (strcmp(channel, "room_stats") == 0) {
    if (doc.containsKey("temperature")) {
      int t = doc["temperature"].as<int>();
      int h = doc["humidity"].as<int>();
      int p = doc["pressure"].as<int>();
      currentTemp = t;
      Serial.printf("[room_stats] T: %d°C  H: %d%%  P: %d hPa\n", t, h, p);
      updateDisplay();
      checkTemperatureControl();
    }
  } else if (strcmp(channel, "air_conditioning") == 0) {
    if (doc.containsKey("temperature")) {
      int t = doc["temperature"].as<int>();
      currentTemp = t;
      Serial.printf("[air_conditioning] Ambient temperature: %d°C\n", t);
      updateDisplay();
      checkTemperatureControl();
    }
    if (doc.containsKey("requestedTemp")) {
      int reqTemp = doc["requestedTemp"].as<int>();
      requestedTemp = reqTemp;
      manualOverride = false;
      Serial.printf("[air_conditioning] Requested temperature: %d°C\n", reqTemp);
      updateDisplay();
      checkTemperatureControl();
    }
    if (doc.containsKey("klimaStatus")) {
      const char* st = doc["klimaStatus"];
      if (st && (strcmp(st, "ON") == 0 || strcmp(st, "OFF") == 0)) {
        bool newState = (strcmp(st, "ON") == 0);
        if (newState != klimaOn) {
          klimaOn = newState;
          if (newState) {
            manualOverride = false;
            checkTemperatureControl();
          } else {
            manualOverride = true;
            strcpy(currentFunction, "");
          }
          updateDisplay();
          sendWebSocketData();
          Serial.printf("[RX] AC %s (ręczne sterowanie)\n", klimaOn ? "Włączone" : "Wyłączone");
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
  pinMode(buttonPin, INPUT_PULLUP);
  initializeDisplay();

  MyWiFi::connect();

  initializeJSON();

  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      Serial.println("Połączono z WS – identyfikacja kanału air_conditioning");
      webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"air_conditioning\"}");
      sendWebSocketData();
      updateDisplay();
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
    }
  });
  webSocketClient.setReconnectInterval(RECONNECT_INTERVAL);
}

void loop() {
  webSocketClient.loop();
  static bool lastButton = HIGH;
  bool reading = digitalRead(buttonPin);
  
  if (reading == LOW && lastButton == HIGH) {
    klimaOn = !klimaOn;
    manualOverride = true;
    
    if (!klimaOn) {
      strcpy(currentFunction, "");
    } else {
      manualOverride = false;
      checkTemperatureControl();
      manualOverride = true;
    }
    
    Serial.printf("Przycisk naciśnięty - AC %s (ręczne)\n", klimaOn ? "ON" : "OFF");
    updateDisplay();
    sendWebSocketData();
    delay(200);
  }

  lastButton = reading;
}