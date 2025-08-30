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
const char* WIFI_AP_NAME = "Główne Światła AP";
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;  // 5 sekund
const unsigned long SERIAL_BAUD_RATE = 19200;

// Konfiguracja przekaźnika i zmiennych specyficznych dla tego urządzenia
const int RELAY_PIN = 26;                    // Pin do którego podłączony jest przekaźnik
const int TOUCH_BUTTON_PIN = 23;             // Pin do którego podłączony jest przycisk dotykowy
const bool ACTIVE_LOW = true;                // Czy przekaźnik jest aktywny na LOW
int lastLightState = HIGH;                   // Ostatni stan światła
int lastButtonState = HIGH;                  // Ostatni stan przycisku (HIGH = nie wciśnięty z INPUT_PULLUP)

// Obiekty do komunikacji
WebSocketsClient webSocketClient;            // Klient WebSocket do wysyłania danych w czasie rzeczywistym
WebServer server(HTTP_SERVER_PORT);          // Serwer HTTP do obsługi zapytań REST

// Globalny dokument JSON do przechowywania stanu światła
StaticJsonDocument<100> jsonPayload;

// Inicjalizuje strukturę JSON z podstawowymi polami
void initializeJSON() {
  jsonPayload["channel"] = "mainLights";     // Kanał identyfikujący typ danych
  jsonPayload["lightStatus"] = "";           // Status światła (będzie wypełniony później)
}

// Aktualizuje status światła w globalnym JSON na podstawie stanu przekaźnika
void updateJSONData(int lightState) {
  if (ACTIVE_LOW) {
    // Jeśli przekaźnik jest aktywny na LOW, odwracamy logikę
    if (lightState == LOW) {
      jsonPayload["lightStatus"] = "ON";     // LOW = światło włączone
    } else {
      jsonPayload["lightStatus"] = "OFF";    // HIGH = światło wyłączone
    }
  } else {
    // Standardowa logika
    if (lightState == HIGH) {
      jsonPayload["lightStatus"] = "ON";     // HIGH = światło włączone
    } else {
      jsonPayload["lightStatus"] = "OFF";    // LOW = światło wyłączone
    }
  }
}

// Przełącza stan świateł (toggle)
void toggleLights() {
  int currentState = digitalRead(RELAY_PIN);
  int newState = (currentState == HIGH) ? LOW : HIGH;
  
  digitalWrite(RELAY_PIN, newState);
  updateJSONData(newState);
  sendWebSocketData();
  lastLightState = newState;
  
  Serial.println("Lights toggled to: " + String(jsonPayload["lightStatus"].as<String>()));
}

// Ustawia konkretny stan świateł
void setLightState(String state) {
  int targetState;
  
  if (ACTIVE_LOW) {
    targetState = (state == "ON") ? LOW : HIGH;
  } else {
    targetState = (state == "ON") ? HIGH : LOW;
  }
  
  digitalWrite(RELAY_PIN, targetState);
  updateJSONData(targetState);
  lastLightState = targetState;
  
  Serial.println("Lights set to: " + state);
}


// Wysyła dane JSON przez WebSocket do serwera
// Używane do aktualizacji w czasie rzeczywistym gdy stan światła się zmieni
void sendWebSocketData() {
  String jsonStr;
  serializeJson(jsonPayload, jsonStr);       // Konwertuje JSON na string
  webSocketClient.sendTXT(jsonStr);          // Wysyła przez WebSocket
  Serial.println("Sent: " + jsonStr);       // Debug: wyświetla wysłane dane
}

// Obsługuje zapytania HTTP GET na endpoint /status
// Używane przez backend PHP do pobrania aktualnego stanu światła po odświeżeniu strony
void handleStatusRequest() {
  int currentLightState = digitalRead(RELAY_PIN);
  delay(20);                                 
  currentLightState = digitalRead(RELAY_PIN);
  updateJSONData(currentLightState);
  
  // Tworzy odpowiedź JSON tylko ze statusem (bez kanału)
  StaticJsonDocument<100> responseDoc;
  responseDoc["lightStatus"] = jsonPayload["lightStatus"];
  String response;
  serializeJson(responseDoc, response);
  
  server.send(200, "application/json", response);
  Serial.println("HTTP light status sent: " + response);
}





void setup() {
  Serial.begin(SERIAL_BAUD_RATE);            // Inicjalizacja komunikacji szeregowej do debugowania
  WiFiManager wm;                            // Manager automatycznego połączenia WiFi
  wm.autoConnect(WIFI_AP_NAME);              // Tworzy punkt dostępu jeśli nie może się połączyć

  Serial.println("Połączono z siecią WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());            // Wyświetla IP ESP32 w sieci

  pinMode(RELAY_PIN, OUTPUT);                // Konfiguruje pin jako wyjście dla przekaźnika
  pinMode(TOUCH_BUTTON_PIN, INPUT_PULLUP);   // Konfiguruje pin przycisku z rezystorem podciągającym
  
  // Inicjalizacja JSON i odczyt początkowego stanu
  initializeJSON();
  lastLightState = digitalRead(RELAY_PIN);
  lastButtonState = digitalRead(TOUCH_BUTTON_PIN);
  updateJSONData(lastLightState);

  // Konfiguracja serwera HTTP
  server.on("/status", HTTP_GET, handleStatusRequest);     // Endpoint dla zapytań o status
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
          
          // Identyfikuj się jako ESP32 main lights controller
          webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"mainLights\"}");
          Serial.println("ESP32 identification sent");
          
          // Połączenie nawiązane po restarcie/ponownym podłączeniu - wysyłamy aktualny stan
          int currentLightState = digitalRead(RELAY_PIN);
          updateJSONData(currentLightState);
          sendWebSocketData();
          lastLightState = currentLightState;
          Serial.println("Initial light status sent after reconnection");
          break;
        }
      case WStype_TEXT:
        {
          // Odebrano wiadomość tekstową z serwera
          String message = String((char*)payload);
          Serial.println("Received WebSocket message: " + message);
          
          // Parsuj JSON
          StaticJsonDocument<200> receivedDoc;
          DeserializationError error = deserializeJson(receivedDoc, message);
          
          if (!error && receivedDoc["channel"] == "mainLights") {
            String newState = receivedDoc["lightStatus"];
            String currentState = jsonPayload["lightStatus"];
            
            if (newState != currentState && (newState == "ON" || newState == "OFF")) {
              setLightState(newState);
              Serial.println("Lights state changed via WebSocket: " + newState);
            }
          }
          break;
        }    
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);    // Automatyczne ponowne łączenie
} 


void loop() {
  server.handleClient();                     // Obsługuje przychodzące zapytania HTTP
  webSocketClient.loop();                    // Utrzymuje połączenie WebSocket
  
  // Sprawdza stan przycisku dotykowego
  int currentButtonState = digitalRead(TOUCH_BUTTON_PIN);
  
  // Sprawdza czy przycisk został wciśnięty (zmiana z HIGH na LOW)
  if (currentButtonState != lastButtonState) {
    delay(50);                               // Debouncing
    currentButtonState = digitalRead(TOUCH_BUTTON_PIN);
    if (currentButtonState != lastButtonState) {
      if (currentButtonState == LOW) {        // Przycisk wciśnięty
        toggleLights();                       // Przełącz stan świateł
        Serial.println("Touch button pressed - lights toggled");
      }
      lastButtonState = currentButtonState;   // Zapamiętuje nowy stan przycisku
    }
  }
  
  // Sprawdza czy stan światła się zmienił (backup - w przypadku zewnętrznych zmian)
  int currentLightState = digitalRead(RELAY_PIN);
  if (currentLightState != lastLightState) {
    delay(20);                               
    currentLightState = digitalRead(RELAY_PIN);
    if (currentLightState != lastLightState) {
      updateJSONData(currentLightState);
      sendWebSocketData();                   // Wysyła aktualizację przez WebSocket
      lastLightState = currentLightState;    // Zapamiętuje nowy stan
      Serial.println("External light state change detected");
    }
  }
}