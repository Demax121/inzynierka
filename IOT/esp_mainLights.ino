// ========== BIBLIOTEKI ==========
#include <WiFiManager.h>       // Automatyczne zarządzanie połączeniem WiFi
#include <WiFi.h>              // Podstawowe funkcje WiFi
#include <ArduinoJson.h>       // Tworzenie i parsowanie JSON
#include <WebSocketsClient.h>  // Klient WebSocket do komunikacji real-time
#include <WebServer.h>         // Serwer HTTP do obsługi zapytań REST

// ========== GLOBALNE KONFIGURACJE ==========

const unsigned long SERIAL_BAUD_RATE = 19200;

const int RELAY_PIN = 27;            // Sterowanie przekaźnikiem (ACTIVE = LOW po zmianie okablowania COM+NO)
const int TOUCH_BUTTON_PIN = 22;     // Wejście przycisku / sensora dotykowego (aktywny HIGH)

// Poziomy logiczne – w razie zmiany hardware wystarczy dostosować stałe
const int RELAY_ACTIVE_LEVEL = LOW;      // Poziom logiczny ZAŁĄCZAJĄCY (moduł active LOW)
const int RELAY_INACTIVE_LEVEL = HIGH;   // Poziom logiczny WYŁĄCZAJĄCY
const int BUTTON_ACTIVE_LEVEL = HIGH;    // Odczyt HIGH oznacza dotknięcie / żądanie zmiany

// Debounce
const unsigned long BUTTON_DEBOUNCE_MS = 25;  // Krótki czas filtracji drgań

bool State = false;                 // Logiczny stan światła: true = ON
int lastButtonState = LOW;          // Poprzedni stabilny odczyt przycisku
unsigned long lastButtonChangeMs = 0;  // Czas ostatniego przełączenia (debounce)

const char* WEBSOCKET_SERVER = "192.168.1.4";
const int WEBSOCKET_PORT = 8886;
const int HTTP_SERVER_PORT = 80;
const char* WIFI_AP_NAME = "Główne Światła AP";
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;  // 5 sekund


WebSocketsClient webSocketClient;
WebServer server(HTTP_SERVER_PORT);

StaticJsonDocument<100> jsonPayload;  // Zawiera channel + lightStatus


void initializeJSON() {
  jsonPayload["channel"] = "mainLights";
  jsonPayload["lightStatus"] = ""; // Wypełniane przy pierwszym updateJSONData
}

// Aktualizuje pole lightStatus na podstawie stanu przekaźnika
void updateJSONData(int relayState) {
  jsonPayload["lightStatus"] = (relayState == RELAY_ACTIVE_LEVEL) ? "ON" : "OFF";
}

// Ustawia stan przekaźnika oraz aktualizuje JSON poprzez updateJSONData
void setRelay(bool on) {
  State = on;
  digitalWrite(RELAY_PIN, on ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL);
  int relayState = digitalRead(RELAY_PIN); // Odczyt rzeczywisty
  updateJSONData(relayState);
}

void sendWebSocketData() {
  String jsonStr;
  serializeJson(jsonPayload, jsonStr);       // Konwertuje JSON na string
  webSocketClient.sendTXT(jsonStr);          // Wysyła przez WebSocket
  Serial.println("Sent: " + jsonStr);        // Debug: wyświetla wysłane dane
}

// Dodana brakująca funkcja handleStatusRequest
void handleStatusRequest() {
  // Odczyt z pinu dla pewności (gdyby ktoś manualnie zmienił stan hardware) i synchronizacja
  int pinState = digitalRead(RELAY_PIN);
  bool on = (pinState == RELAY_ACTIVE_LEVEL);
  if (on != State) {              // Jeżeli różnica – zsynchronizuj strukturę + JSON
    setRelay(on);
  } else {
    updateJSONData(pinState);     // Odśwież JSON jeśli brak zmian sprzętowych
  }

  StaticJsonDocument<64> responseDoc;
  responseDoc["lightStatus"] = jsonPayload["lightStatus"];
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
  Serial.println("HTTP light status sent: " + response);
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);  // Inicjalizacja komunikacji szeregowej do debugowania

  // Wczesna inicjalizacja przekaźnika aby uniknąć glitch (najpierw ustaw latch, potem OUTPUT)
  digitalWrite(RELAY_PIN, RELAY_INACTIVE_LEVEL); // Gwarantowane OFF na starcie (active LOW)
  pinMode(RELAY_PIN, OUTPUT);

  pinMode(TOUCH_BUTTON_PIN, INPUT); // Jeśli kiedyś potrzebny będzie wewnętrzny pullup -> INPUT_PULLUP

  initializeJSON();
  setRelay(false); // Ustal jasny stan OFF + JSON

  WiFiManager wm;                  // Manager automatycznego połączenia WiFi
  wm.autoConnect(WIFI_AP_NAME);    // Tworzy punkt dostępu jeśli nie może się połączyć

  Serial.println("Połączono z siecią WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());  // Wyświetla IP ESP32 w sieci

  // Konfiguracja serwera HTTP
  server.on("/status", HTTP_GET, handleStatusRequest);  // Endpoint dla zapytań o status
  server.begin();
  Serial.println("HTTP server started");

  // Konfiguracja klienta WebSocket
  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");  // Połączenie z serwerem WebSocket
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
      case WStype_CONNECTED: {
        Serial.printf("WebSocket Connected to: %s\n", payload);
        // Identyfikacja urządzenia
        webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"mainLights\"}");
        Serial.println("ESP32 identification sent");
        // Wyślij aktualny znany stan
        sendWebSocketData();
        break;
      }
      case WStype_TEXT: {
        String message = String((char*)payload);
        Serial.println("Received: " + message);
        StaticJsonDocument<200> doc;
        DeserializationError err = deserializeJson(doc, message);
        if (err) {
          Serial.println("JSON parse error, ignored");
          break;
        }
        if (doc["channel"] == "mainLights" && doc.containsKey("lightStatus")) {
          String command = doc["lightStatus"];
          if (command == "ON") {
            setRelay(true);
            Serial.println("Light turned ON via WebSocket");
          } else if (command == "OFF") {
            setRelay(false);
            Serial.println("Light turned OFF via WebSocket");
          }
          sendWebSocketData();
        }
        break;
      }
      default:
        break;
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);    // Automatyczne ponowne łączenie
}

void loop() {
  server.handleClient();          // Obsługa HTTP
  webSocketClient.loop();         // Obsługa WebSocket

  int rawButton = digitalRead(TOUCH_BUTTON_PIN);

  // Detekcja zbocza LOW -> HIGH z debounce
  if (rawButton == BUTTON_ACTIVE_LEVEL && lastButtonState != BUTTON_ACTIVE_LEVEL) {
    unsigned long now = millis();
    if (now - lastButtonChangeMs >= BUTTON_DEBOUNCE_MS) {
      // Potwierdź po krótkiej zwłoce (opcjonalne drugie sprawdzenie)
      delay(BUTTON_DEBOUNCE_MS);
      if (digitalRead(TOUCH_BUTTON_PIN) == BUTTON_ACTIVE_LEVEL) {
        lastButtonChangeMs = now;
        setRelay(!State);
        sendWebSocketData();
        Serial.println(State ? "Light turned ON via button" : "Light turned OFF via button");
      }
    }
  }

  lastButtonState = rawButton;
}