#include <MyWiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

// Konfiguracja SSL WebSocket
String WEBSOCKET_SERVER = "simply.smart";
const int WEBSOCKET_PORT = 443;  // Port HTTPS
const char* WEBSOCKET_PATH = "/ws";
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;

const int BUTTON_PIN = 21;
int lastButtonState = HIGH;
const unsigned long DEBOUNCE_MS = 30;
unsigned long lastChangeTime = 0;

WebSocketsClient webSocketClient;
StaticJsonDocument<256> jsonPayload;
bool doorOpen = false;

// Watchdog WebSocket
unsigned long lastWsConnected = 0;
unsigned long lastWsAttempt = 0;
const unsigned long WS_RECONNECT_TIMEOUT = 15000;

void initializeJSON() { 
  jsonPayload["identity"] = "main_door_sensor";
  jsonPayload["channel"] = "door_sensor";
  JsonObject payload = jsonPayload.createNestedObject("payload");
  payload["doorOpen"] = false;
}

void updateJSONData(int buttonState) { 
  doorOpen = (buttonState == HIGH);
  JsonObject payload = jsonPayload["payload"];
  payload["doorOpen"] = doorOpen;
}

void identifyDevice() {
  StaticJsonDocument<128> idDoc;
  idDoc["type"] = "esp32_identification";
  idDoc["channel"] = "door_sensor";
  String idMessage;
  serializeJson(idDoc, idMessage);
  webSocketClient.sendTXT(idMessage);
}

void sendWebSocketData() {
  if (!webSocketClient.isConnected()) return;
  String jsonStr;
  serializeJson(jsonPayload, jsonStr);
  webSocketClient.sendTXT(jsonStr);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      lastWsConnected = millis();
      
      // Wyślij identyfikację urządzenia po połączeniu
      identifyDevice();
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
    case WStype_ERROR:
      Serial.printf("[WSc] Error: %s\n", payload);
      break;
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

void setupWebSocket() {
  // **KLUCZOWA CZĘŚĆ - KONFIGURACJA SSL**
  
  // Uzyskaj certyfikat z MyWiFi
  const char* cert = MyWiFi::getCert();
  
  // Skonfiguruj SSL WebSocket (zgodnie z oficjalnym przykładem)
  webSocketClient.beginSSL(
    WEBSOCKET_SERVER.c_str(),   // Adres serwera
    WEBSOCKET_PORT,             // Port HTTPS (443)
    WEBSOCKET_PATH              // Ścieżka WebSocket (/ws)
  );
  
  // Ustaw certyfikat CA dla weryfikacji SSL
  webSocketClient.setCACert(cert);
  
  // Alternatywnie, jeśli chcesz pominąć weryfikację certyfikatu (NIEZALECANE w produkcji):
  // webSocketClient.setInsecure();
  
  webSocketClient.onEvent(webSocketEvent);
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
  
  Serial.println("WebSocket SSL configured");
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Połącz z WiFi
  MyWiFi::connect();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
  
  initializeJSON();
  setupWebSocket();
}

void loop() {
  webSocketClient.loop();
  
  // Odczyt przycisku z debounce
  int currentButtonState = digitalRead(BUTTON_PIN);
  unsigned long currentTime = millis();
  
  if (currentButtonState != lastButtonState && 
      currentTime - lastChangeTime > DEBOUNCE_MS) {
    
    updateJSONData(currentButtonState);
    sendWebSocketData();
    
    Serial.print("Door state changed: ");
    Serial.println(doorOpen ? "OPEN" : "CLOSED");
    
    lastButtonState = currentButtonState;
    lastChangeTime = currentTime;
  }
  
  // Watchdog - sprawdź połączenie WebSocket
  if (!webSocketClient.isConnected() && 
      currentTime - lastWsAttempt > WS_RECONNECT_TIMEOUT) {
    Serial.println("Attempting WebSocket reconnection...");
    setupWebSocket();
    lastWsAttempt = currentTime;
  }
  
  delay(50);
}