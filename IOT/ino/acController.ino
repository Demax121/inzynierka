#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <AESCrypto.h>

#define TFT_CS   5
#define TFT_DC   21
#define TFT_RST  22

const int buttonPin = 13;
bool BTNstate = false;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

WebSocketsClient webSocketClient;

String WEBSOCKET_SERVER = "192.168.1.2";
const int   WEBSOCKET_PORT   = 8884;
const unsigned long RECONNECT_INTERVAL = 5000;
String device_api_key = "NfzziMcV9Zyj";
String encryption_key = "KQAzhJmqigFdUyD6";

bool klimaOn = false;
bool manualOverride = false;

float currentTemp = 0.0;
float requestedTemp = 25.0;
const float HISTERAZA = 2.0;
String currentFunction = "";

AESCrypto crypto(encryption_key);

unsigned long lastWsConnected = 0;
unsigned long lastWsAttempt = 0;
const unsigned long WS_RECONNECT_TIMEOUT = 15000;

StaticJsonDocument<256> doc;
StaticJsonDocument<512> jsonPayload; // envelope only used on send

String temp_aktualna = "TEMP AKTUALNA:";
String temp_oczekiwana = "TEMP OCZEKIWANA:";
String status_text = "STATUS:";
String funkcja_text = "FUNKCJA:";
String on_text = "ON";
String off_text = "OFF";
String spoczynku_text = "W SPOCZYNKU";
String space_c = " C";
String dash_c = "-- C";
String chlodzenie_text = "CHLODZIMY!!!";
String grzanie_text = "GRZEJEMY!!!";

void initializeDisplay() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  
  tft.setCursor(10, 20);
  tft.print(temp_aktualna);
  tft.setCursor(10, 100);
  tft.print(temp_oczekiwana);
  tft.setCursor(10, 180);
  tft.print(status_text);
  tft.setCursor(160, 180);
  tft.print(funkcja_text);
}

void updateDisplay() {
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);
  
  tft.fillRect(10, 50, 300, 30, ILI9341_BLACK);
  tft.setCursor(10, 50);
  if (currentTemp > 0.0) {
    tft.print(currentTemp, 1);
    tft.print(space_c);
  } else {
    tft.print(dash_c);
  }
  
  tft.fillRect(10, 130, 300, 30, ILI9341_BLACK);
  tft.setCursor(10, 130);
  tft.print(requestedTemp, 1);
  tft.print(space_c);
  
  tft.fillRect(10, 210, 140, 25, ILI9341_BLACK);
  tft.setCursor(10, 210);
  tft.setTextSize(2);
  if (klimaOn) {
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.print(on_text);
  } else {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.print(off_text);
  }
  
  tft.fillRect(160, 210, 150, 25, ILI9341_BLACK);
  tft.setCursor(160, 210);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setTextSize(2);
  if (currentFunction.length() > 0) {
    tft.print(currentFunction);
  } else {
    tft.print(spoczynku_text);
  }
}

void initializeJSON() {
  // envelope created on send
}

void updateJSONData() {
  JsonObject payload = jsonPayload["payload"];
  payload["requestedTemp"] = requestedTemp;
  payload["function"] = currentFunction;
  payload["klimaON"] = klimaOn;
  payload["manualOverride"] = manualOverride;
}

void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "air_conditioning";
  idDoc["device_api_key"] = device_api_key;
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocketClient.sendTXT(idMessage);
}

void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  updateJSONData();
  StaticJsonDocument<256> p;
  p["requestedTemp"] = requestedTemp;
  p["function"] = currentFunction;
  p["klimaON"] = klimaOn;
  p["manualOverride"] = manualOverride;
  String plain; serializeJson(p, plain);
  String iv = AESCrypto::generateIV(); String cipher = crypto.encrypt(plain, iv);
  StaticJsonDocument<320> env;
  env["identity"] = "air_conditioning";
  env["channel"] = "air_conditioning";
  env["device_api_key"] = device_api_key;
  env["msgIV"] = iv;
  env["payload"] = cipher;
  String out; serializeJson(env, out);
  webSocketClient.sendTXT(out);
  Serial.printf("[TX] AC %s\n", klimaOn ? "Włączone" : "Wyłączone");
}

void checkTemperatureControl() {
  if (currentTemp == 0) return;
  if (manualOverride) return;
  
  float tempDiff = currentTemp - requestedTemp;
  bool previousState = klimaOn;
  String previousFunction = currentFunction;
  if (tempDiff >= HISTERAZA) {
    klimaOn = true;
    currentFunction = chlodzenie_text;
    if (!previousState || previousFunction != currentFunction) {
      Serial.println("CHŁODZIMY!!!");
      sendWebSocketData();
      updateDisplay();
    }
  } else if (tempDiff <= -HISTERAZA) {
    klimaOn = true;
    currentFunction = grzanie_text;
    if (!previousState || previousFunction != currentFunction) {
      Serial.println("GRZEJEMY!!!");
      sendWebSocketData();
      updateDisplay();
    }
  } else {
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
  String msg = "";
  for (size_t i = 0; i < length; i++) msg += (char)payload[i];
  DeserializationError err = deserializeJson(doc, msg);
  if (err) return;
  
  // Handle ping message from server
  if (doc.containsKey("type") && String((const char*)doc["type"]) == "ping") {
    sendWebSocketData();
    return;
  }

  String channel = doc["channel"] | "";
  if (channel != "air_conditioning") return;

  // Encrypted payloads from server for both temperature push and commands
  if (!doc.containsKey("msgIV") || !doc.containsKey("payload")) return;
  String iv = doc["msgIV"].as<String>();
  String cipher = doc["payload"].as<String>();
  String plain = crypto.decrypt(cipher, iv);
  if (plain.length() == 0) return;
  StaticJsonDocument<256> body; if (deserializeJson(body, plain)) return;

  // If contains temperature (room_stats push)
  if (body.containsKey("temperature")) {
    float t = body["temperature"].as<float>();
    currentTemp = t;
    Serial.printf("[air_conditioning] Current temperature: %.1f°C\n", t);
    updateDisplay();
    checkTemperatureControl();
  }

  // Frontend commands
  if (body.containsKey("klimaON")) {
    bool newState = body["klimaON"];
    if (newState != klimaOn) {
      klimaOn = newState;
      if (body.containsKey("manualOverride")) {
        manualOverride = body["manualOverride"];
      }
      if (!newState) {
        currentFunction = "";
      } else if (!manualOverride) {
        checkTemperatureControl();
      }
      updateDisplay();
      sendWebSocketData();
      Serial.printf("[RX] AC %s (manual control)\n", klimaOn ? "ON" : "OFF");
    }
  }

  if (body.containsKey("requestedTemp")) {
    float reqTemp = body["requestedTemp"].as<float>();
    requestedTemp = reqTemp;
    manualOverride = false;
    Serial.printf("[air_conditioning] Requested temperature: %.1f°C\n", reqTemp);
    updateDisplay();
    checkTemperatureControl();
    sendWebSocketData();
  }
}

void handleButton() {
  static bool lastButton = HIGH;
  static unsigned long lastButtonChange = 0;
  const unsigned long BUTTON_DEBOUNCE_MS = 200;
  
  bool reading = digitalRead(buttonPin);
  
  if (reading == LOW && lastButton == HIGH) {
    if (millis() - lastButtonChange > BUTTON_DEBOUNCE_MS) {
      klimaOn = !klimaOn;
      manualOverride = true;
      if (!klimaOn) {
        currentFunction = "";
      } else {
        manualOverride = false;
        checkTemperatureControl();
        manualOverride = true;
      }
      Serial.printf("Button pressed - AC %s (manual)\n", klimaOn ? "ON" : "OFF");
      updateDisplay();
      if (webSocketClient.isConnected()) {
        sendWebSocketData();
      }
      lastButtonChange = millis();
    }
  }

  lastButton = reading;
}

void setup() {
  Serial.begin(19200);
  delay(500);
  pinMode(buttonPin, INPUT_PULLUP);
  initializeDisplay();

  MyWiFi::connect();

  initializeJSON();

  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  lastWsConnected = millis();
  lastWsAttempt = millis();
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      lastWsConnected = millis();
      Serial.println("Connected to WS – identifying air_conditioning channel");
      identifyDevice();
      sendWebSocketData();
      updateDisplay();
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
    }
  });
  webSocketClient.setReconnectInterval(RECONNECT_INTERVAL);
}

void loop() {
  // Handle button at the beginning - independent of WebSocket
  handleButton();
  
  // WebSocket loop - may block during reconnect
  webSocketClient.loop();

  if (!webSocketClient.isConnected()) {
    if (millis() - lastWsConnected > WS_RECONNECT_TIMEOUT && millis() - lastWsAttempt > 5000) {
      Serial.println("WebSocket not responding, restarting connection...");
      webSocketClient.disconnect();
      delay(100);
      webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
      lastWsAttempt = millis();
    }
  }
}

