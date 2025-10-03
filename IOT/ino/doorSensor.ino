#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Arduino.h>
#include <esp_random.h>
#include <AESCrypto.h>

// VARIABLES
String WEBSOCKET_SERVER = "192.168.1.2";
const int WEBSOCKET_PORT = 8884;
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;
String device_api_key = "akr3ZPYDU5rM";
String encryption_key = "2zBxk8uBiQd7p32T";

const int BUTTON_PIN = 21;
int lastButtonState = HIGH;
const unsigned long DEBOUNCE_MS = 30;
unsigned long lastChangeTime = 0;

WebSocketsClient webSocketClient;
bool doorOpen = false;
AESCrypto crypto(encryption_key);

// Watchdog WebSocket
unsigned long lastWsConnected = 0;
unsigned long lastWsAttempt = 0;
const unsigned long WS_RECONNECT_TIMEOUT = 15000;
const unsigned long WS_RETRY_EVERY = 5000;


// ENCRYPT AND DECRYPT BEGIN





// ENCRYPT AND DECRYPT END


// FUNCTIONS BEGIN
void initializeJSON() { 
  // envelope created on send
  doorOpen = false;
}

void updateJSONData(int buttonState) { 
  doorOpen = (buttonState == HIGH);
}

void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "door_sensor";
  idDoc["device_api_key"] = device_api_key;
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocketClient.sendTXT(idMessage);
}

void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  // Build clear payload
  StaticJsonDocument<128> p;
  p["doorOpen"] = doorOpen;
  String plain; serializeJson(p, plain);
  String iv = AESCrypto::generateIV();
  String cipher = crypto.encrypt(plain, iv);
  
  StaticJsonDocument<256> env;
  env["identity"] = "main_door_sensor";
  env["channel"] = "door_sensor";
  env["device_api_key"] = device_api_key;
  env["msgIV"] = iv;
  env["payload"] = cipher;
  String out; serializeJson(env, out);
  webSocketClient.sendTXT(out);
}

void handleIncomingText(uint8_t* payload, size_t length) {
  String msg = "";
  for (size_t i = 0; i < length; i++) msg += (char)payload[i];
  
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) return;
  
  // Handle ping message from server (unencrypted)
  if (doc.containsKey("type") && String((const char*)doc["type"]) == "ping") {
    updateJSONData(digitalRead(BUTTON_PIN));
    sendWebSocketData();
  }
  
  // Handle other unencrypted messages if needed in the future
}

// FUNCTIONS END

void setup() {
  Serial.begin(19200);
  
  MyWiFi::connect();
  // dalsze próby łączenia obsłuży MyWiFi::loop() w pętli głównej

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  initializeJSON();
  lastButtonState = digitalRead(BUTTON_PIN);
  updateJSONData(lastButtonState);
  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      lastWsConnected = millis(); // aktualizuj czas połączenia
      identifyDevice();
      int currentButtonState = digitalRead(BUTTON_PIN);
      updateJSONData(currentButtonState);
      sendWebSocketData();
      lastButtonState = currentButtonState;
    } else if (type == WStype_TEXT) {
      handleIncomingText(payload, length);
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
  lastWsConnected = millis();
  lastWsAttempt = millis();
}

void loop() {
  // Utrzymanie połączenia WiFi (nieblokujące)
  MyWiFi::loop();

  // Jeśli nie ma WiFi, nie próbuj obsługiwać WebSocket ani odczytu zdarzeń (redukcja hałasu)
  if (!MyWiFi::isConnected()) {
    delay(10);
    return;
  }

  webSocketClient.loop();
  int currentState = digitalRead(BUTTON_PIN);
  if (currentState != lastButtonState) {
    if (lastChangeTime == 0) {
      lastChangeTime = millis();
    } else if (millis() - lastChangeTime >= DEBOUNCE_MS) {
      lastButtonState = currentState;
      updateJSONData(lastButtonState);
      sendWebSocketData();
      lastChangeTime = 0;
    }
  } else {
    lastChangeTime = 0;
  }

  // Watchdog WebSocket
  if (!webSocketClient.isConnected()) {
    if (millis() - lastWsConnected > WS_RECONNECT_TIMEOUT && millis() - lastWsAttempt > WS_RETRY_EVERY) {
      Serial.println("WebSocket nie odpowiada, restart połączenia...");
      webSocketClient.disconnect();
      delay(100);
      // upewnij się, że próbujemy na tym samym path jak na początku
      webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
      lastWsAttempt = millis();
    }
  }
}