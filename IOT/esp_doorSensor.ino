// Biblioteki potrzebne do obsługi ESP32
#include <WiFiManager.h>      // Automatyczne zarządzanie połączeniem WiFi
#include <WiFi.h>             // Podstawowe funkcje WiFi
#include <ArduinoJson.h>      // Tworzenie i parsowanie JSON
#include <WebSocketsClient.h> // Klient WebSocket do komunikacji real-time
#include <WebServer.h>        // Serwer HTTP do obsługi zapytań REST

// ========== GLOBALNE KONFIGURACJE ==========
// Konfiguracja sieci
const char* WEBSOCKET_SERVER = "192.168.1.4";
const int WEBSOCKET_PORT = 8886;
const int HTTP_SERVER_PORT = 80;
const char* WIFI_AP_NAME = "Sensor Drzwi AP";
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;  // 5 sekund
const unsigned long SERIAL_BAUD_RATE = 19200;

// Konfiguracja pinu i zmiennych specyficznych dla tego urządzenia
const int BUTTON_PIN = 10;                    // Pin do którego podłączony jest przycisk/czujnik drzwi
int lastButtonState = HIGH;                   // Ostatni stan przycisku (HIGH = drzwi otwarte z INPUT_PULLUP)

// Obiekty do komunikacji
WebSocketsClient webSocketClient;            // Klient WebSocket do wysyłania danych w czasie rzeczywistym
WebServer server(HTTP_SERVER_PORT);          // Serwer HTTP do obsługi zapytań REST

// Globalny dokument JSON do przechowywania stanu drzwi
StaticJsonDocument<100> jsonPayload;

// Inicjalizuje strukturę JSON z podstawowymi polami
void initializeJSON() {
  jsonPayload["channel"] = "doorStatus";     // Kanał identyfikujący typ danych
  jsonPayload["status"] = "";                // Status drzwi (będzie wypełniony później)
}

// Aktualizuje status drzwi w globalnym JSON na podstawie stanu przycisku
void updateJSONData(int buttonState) {
  if (buttonState == LOW) {
    jsonPayload["status"] = "zamkniete";     // Przycisk wciśnięty = drzwi zamknięte
  } else {
    jsonPayload["status"] = "otwarte";       // Przycisk nie wciśnięty = drzwi otwarte
  }
}

// Wysyła dane JSON przez WebSocket do serwera
// Używane do aktualizacji w czasie rzeczywistym gdy stan drzwi się zmieni
void sendWebSocketData() {
  String jsonStr;
  serializeJson(jsonPayload, jsonStr);       // Konwertuje JSON na string
  webSocketClient.sendTXT(jsonStr);          // Wysyła przez WebSocket
  Serial.println("Sent: " + jsonStr);       // Debug: wyświetla wysłane dane
}

// Obsługuje zapytania HTTP GET na endpoint /status
// Używane przez backend PHP do pobrania aktualnego stanu drzwi po odświeżeniu strony
void handleStatusRequest() {
  int currentButtonState = digitalRead(BUTTON_PIN);
  delay(20);                                 
  currentButtonState = digitalRead(BUTTON_PIN);
  updateJSONData(currentButtonState);
  
  // Tworzy odpowiedź JSON tylko ze statusem (bez kanału)
  StaticJsonDocument<100> responseDoc;
  responseDoc["status"] = jsonPayload["status"];
  String response;
  serializeJson(responseDoc, response);
  
  server.send(200, "application/json", response);
  Serial.println("HTTP door status sent: " + response);
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);            // Inicjalizacja komunikacji szeregowej do debugowania
  WiFiManager wm;                            // Manager automatycznego połączenia WiFi
  wm.autoConnect(WIFI_AP_NAME);              // Tworzy punkt dostępu jeśli nie może się połączyć

  Serial.println("Połączono z siecią WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());            // Wyświetla IP ESP32 w sieci

  pinMode(BUTTON_PIN, INPUT_PULLUP);         // Konfiguruje pin z wewnętrznym rezystorem podciągającym

  // Inicjalizacja JSON i odczyt początkowego stanu
  initializeJSON();
  lastButtonState = digitalRead(BUTTON_PIN);
  updateJSONData(lastButtonState);

  // Konfiguracja serwera HTTP
  server.on("/status", HTTP_GET, handleStatusRequest);  // Endpoint dla zapytań o status
  server.begin();
  Serial.println("HTTP server started");

  // Konfiguracja klienta WebSocket
  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");         // Połączenie z serwerem WebSocket
  
  // Obsługa zdarzeń WebSocket - ważne dla synchronizacji po ponownym podłączeniu urządzenia
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
      case WStype_CONNECTED:
        {
          Serial.printf("WebSocket Connected to: %s\n", payload);
          
          // Identyfikuj się jako ESP32 door sensor
          webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"doorStatus\"}");
          Serial.println("ESP32 identification sent");
          
          // Połączenie nawiązane po restarcie/ponownym podłączeniu - wysyłamy aktualny stan
          int currentButtonState = digitalRead(BUTTON_PIN);
          updateJSONData(currentButtonState);
          sendWebSocketData();
          lastButtonState = currentButtonState;
          Serial.println("Initial door status sent after reconnection");
          break;
        }    
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);    // Automatyczne ponowne łączenie
}

void loop() {
  server.handleClient();                     // Obsługuje przychodzące zapytania HTTP
  webSocketClient.loop();                    // Utrzymuje połączenie WebSocket
  int currentButtonState = digitalRead(BUTTON_PIN);

  // Sprawdza czy stan przycisku się zmienił
  if (currentButtonState != lastButtonState) {
    delay(50);
    currentButtonState = digitalRead(BUTTON_PIN);
    if (currentButtonState != lastButtonState) {
      updateJSONData(currentButtonState);
      sendWebSocketData();                   // Wysyła aktualizację przez WebSocket
      lastButtonState = currentButtonState;  // Zapamiętuje nowy stan
    }
  }
}
