// Biblioteki potrzebne do obsługi ESP32
#include <WiFiManager.h>      // Automatyczne zarządzanie połączeniem WiFi
#include <WiFi.h>             // Podstawowe funkcje WiFi
#include <ArduinoJson.h>      // Tworzenie i parsowanie JSON
#include <Wire.h>             // Komunikacja I2C (nie używane w tym kodzie)
#include <Arduino.h>          // Podstawowe funkcje Arduino
#include <WebSocketsClient.h> // Klient WebSocket do komunikacji real-time
#include <WebServer.h>        // Serwer HTTP do obsługi zapytań REST

// Konfiguracja pinu i zmiennych
const int buttonPin = 10;                    // Pin do którego podłączony jest przycisk/czujnik drzwi
int lastButtonState = HIGH;                  // Ostatni stan przycisku (HIGH = drzwi otwarte z INPUT_PULLUP)

// Obiekty do komunikacji
WebSocketsClient webSocketClient;            // Klient WebSocket do wysyłania danych w czasie rzeczywistym
WebServer server(80);                        // Serwer HTTP na porcie 80 do obsługi zapytań REST

// Globalny dokument JSON do przechowywania stanu drzwi
StaticJsonDocument<100> JSONpayload;

// Inicjalizuje strukturę JSON z podstawowymi polami
void initializeJSON() {
  JSONpayload["channel"] = "doorStatus";     // Kanał identyfikujący typ danych
  JSONpayload["status"] = "";                // Status drzwi (będzie wypełniony później)
}

// Aktualizuje status drzwi w globalnym JSON na podstawie stanu przycisku
void updateJSONWithDoorStatus(int buttonState) {
  // Logika dla INPUT_PULLUP: HIGH = przycisk nie wciśnięty (drzwi otwarte), LOW = wciśnięty (zamknięte)
  if (buttonState == LOW) {
    JSONpayload["status"] = "zamkniete";     // Przycisk wciśnięty = drzwi zamknięte
  } else {
    JSONpayload["status"] = "otwarte";       // Przycisk nie wciśnięty = drzwi otwarte
  }
}

// Wysyła dane JSON przez WebSocket do serwera
// Używane do aktualizacji w czasie rzeczywistym gdy stan drzwi się zmieni
void sendWebSocketData() {
  String jsonStr;
  serializeJson(JSONpayload, jsonStr);       // Konwertuje JSON na string
  webSocketClient.sendTXT(jsonStr);          // Wysyła przez WebSocket
  Serial.println("Sent: " + jsonStr);       // Debug: wyświetla wysłane dane
}

// Obsługuje zapytania HTTP GET na endpoint /status
// Używane przez backend PHP do pobrania aktualnego stanu drzwi po odświeżeniu strony
void handleDoorStatusRequest() {
  int currentButtonState = digitalRead(buttonPin);
  delay(20);                                 // Debouncing - eliminuje zakłócenia z przycisku
  currentButtonState = digitalRead(buttonPin);
  
  updateJSONWithDoorStatus(currentButtonState);
  
  // Tworzy odpowiedź JSON tylko ze statusem (bez kanału)
  StaticJsonDocument<100> responseDoc;
  responseDoc["status"] = JSONpayload["status"];
  
  String response;
  serializeJson(responseDoc, response);
  
  server.send(200, "application/json", response);
  Serial.println("HTTP door status sent: " + response);
}

void setup() {
  Serial.begin(19200);                       // Inicjalizacja komunikacji szeregowej do debugowania
  WiFiManager wm;                            // Manager automatycznego połączenia WiFi
  wm.autoConnect("AutoConnectAP");           // Tworzy punkt dostępu jeśli nie może się połączyć

  Serial.println("Połączono z siecią WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());            // Wyświetla IP ESP32 w sieci

  pinMode(buttonPin, INPUT_PULLUP);          // Konfiguruje pin z wewnętrznym rezystorem podciągającym

  // Inicjalizacja JSON i odczyt początkowego stanu
  initializeJSON();
  lastButtonState = digitalRead(buttonPin);
  updateJSONWithDoorStatus(lastButtonState);

  // Konfiguracja serwera HTTP
  server.on("/status", HTTP_GET, handleDoorStatusRequest);  // Endpoint dla zapytań o status
  server.begin();
  Serial.println("HTTP server started");

  // Konfiguracja klienta WebSocket
  webSocketClient.begin("192.168.1.4", 8886, "/");         // Połączenie z serwerem WebSocket
  
  // Obsługa zdarzeń WebSocket - ważne dla synchronizacji po ponownym podłączeniu urządzenia
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
      case WStype_CONNECTED:
        {
          Serial.printf("WebSocket Connected to: %s\n", payload);
          // Połączenie nawiązane po restarcie/ponownym podłączeniu - wysyłamy aktualny stan
          int currentButtonState = digitalRead(buttonPin);
          updateJSONWithDoorStatus(currentButtonState);
          sendWebSocketData();
          lastButtonState = currentButtonState;
          Serial.println("Initial door status sent after reconnection");
          break;
        }    
    }
  });
  webSocketClient.setReconnectInterval(5000);               // Automatyczne ponowne łączenie co 5s
}

void loop() {
  server.handleClient();                     // Obsługuje przychodzące zapytania HTTP
  webSocketClient.loop();                    // Utrzymuje połączenie WebSocket

  int currentButtonState = digitalRead(buttonPin);

  // Sprawdza czy stan przycisku się zmienił
  if (currentButtonState != lastButtonState) {
    delay(50);                               // Debouncing - czeka na ustabilizowanie sygnału
    currentButtonState = digitalRead(buttonPin);

    // Potwierdza zmianę stanu po debouncing
    if (currentButtonState != lastButtonState) {
      updateJSONWithDoorStatus(currentButtonState);
      sendWebSocketData();                   // Wysyła aktualizację przez WebSocket
      lastButtonState = currentButtonState;  // Zapamiętuje nowy stan
    }
  }
}
