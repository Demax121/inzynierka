// Biblioteki potrzebne do obsługi ESP32 i czujnika BME280
#include <Wire.h>             // Komunikacja I2C z czujnikiem BME280
#include <Adafruit_Sensor.h>  // Podstawowa biblioteka dla czujników Adafruit
#include <Adafruit_BME280.h>  // Obsługa czujnika BME280 (temperatura, wilgotność, ciśnienie)
#include <WiFiManager.h>      // Automatyczne zarządzanie połączeniem WiFi
#include <ArduinoJson.h>      // Tworzenie i parsowanie JSON
#include <WebServer.h>        // Serwer HTTP do obsługi zapytań REST
#include <WiFi.h>             // Podstawowe funkcje WiFi
#include <WebSocketsClient.h> // Klient WebSocket do komunikacji real-time

// ========== GLOBALNE KONFIGURACJE ==========
// Konfiguracja sieci
const char* WEBSOCKET_SERVER = "192.168.1.4";
const int WEBSOCKET_PORT = 8886;
const int HTTP_SERVER_PORT = 80;
const char* WIFI_AP_NAME = "Statystyki Pomieszczenia AP";
const unsigned long WEBSOCKET_RECONNECT_INTERVAL = 5000;  // 5 sekund
const unsigned long SERIAL_BAUD_RATE = 19200;

// Konfiguracja pinów I2C i czujnika
#define SDA_PIN 4                            // Pin SDA dla komunikacji I2C
#define SCL_PIN 5                            // Pin SCL dla komunikacji I2C
const uint8_t BME280_I2C_ADDRESS = 0x76;     // Adres I2C czujnika BME280
const unsigned long SENSOR_SEND_INTERVAL = 10000;   // Interwał wysyłania danych - 10 sekund

// Obiekty do komunikacji
WebServer server(HTTP_SERVER_PORT);         // Serwer HTTP do obsługi zapytań REST
Adafruit_BME280 bme;                         // Obiekt czujnika BME280 (I2C)
WebSocketsClient webSocketClient;            // Klient WebSocket do wysyłania danych w czasie rzeczywistym

// Zmienne czasowe
unsigned long lastSendTime = 0;              // Czas ostatniego wysłania danych przez WebSocket

// Globalny dokument JSON do przechowywania danych z czujników
StaticJsonDocument<200> jsonPayload;

// Inicjalizuje strukturę JSON z podstawowymi polami dla danych z czujników
void initializeJSON() {
  jsonPayload["channel"] = "roomStats";      // Kanał identyfikujący typ danych (statystyki pomieszczenia)
  jsonPayload["temperature"] = "";           // Temperatura (będzie wypełniona później)
  jsonPayload["humidity"] = "";              // Wilgotność (będzie wypełniona później)
  jsonPayload["pressure"] = "";              // Ciśnienie (będzie wypełnione później)
}

// Aktualizuje globalny JSON z aktualnymi danymi z czujnika BME280
void updateJSONData() {
  // Bezpośredni odczyt z czujnika i formatowanie danych z jednostkami
  jsonPayload["temperature"] = String(bme.readTemperature(), 0) + " °C";        // Temperatura w °C (0 miejsc po przecinku)
  jsonPayload["humidity"] = String(bme.readHumidity(), 0) + " %";               // Wilgotność w % (0 miejsc po przecinku)
  jsonPayload["pressure"] = String(bme.readPressure() / 100.0F, 0) + " hPa";    // Ciśnienie w hPa (konwersja z Pa)
}

// Obsługuje zapytania HTTP GET na endpoint /sensor
// Używane przez backend PHP do pobrania aktualnych danych z czujników po odświeżeniu strony
void handleSensorRequest() {
  // Tworzy lokalny JSON z aktualnymi danymi (bez kanału - tylko dane)
  StaticJsonDocument<200> doc;
  doc["temperature"] = String(bme.readTemperature(), 0) + " °C";
  doc["humidity"] = String(bme.readHumidity(), 0) + " %";
  doc["pressure"] = String(bme.readPressure() / 100.0F, 0) + " hPa";

  String output;
  serializeJson(doc, output);                // Konwertuje JSON na string

  server.send(200, "application/json", output);  // Wysyła odpowiedź HTTP z danymi JSON
}

// Wysyła dane JSON przez WebSocket do serwera
// Używane do automatycznych aktualizacji co 10 sekund
void sendWebSocketData() {
  String jsonStr;
  serializeJson(jsonPayload, jsonStr);       // Konwertuje globalny JSON na string
  webSocketClient.sendTXT(jsonStr);          // Wysyła przez WebSocket
  Serial.println("Sent: " + jsonStr);       // Debug: wyświetla wysłane dane
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);            // Inicjalizacja komunikacji szeregowej do debugowania
  WiFiManager wm;                            // Manager automatycznego połączenia WiFi
  Wire.begin(SDA_PIN, SCL_PIN);              // Inicjalizacja komunikacji I2C z określonymi pinami
  wm.autoConnect(WIFI_AP_NAME);              // Tworzy punkt dostępu jeśli nie może się połączyć

  Serial.println("Połączono z siecią WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());            // Wyświetla IP ESP32 w sieci
  
  // Inicjalizacja czujnika BME280
  bool status = bme.begin(BME280_I2C_ADDRESS);  // Próba połączenia z czujnikiem
  if (!status) {
    Serial.println("Nie wykryto BME280!");   // Błąd - czujnik nie został znaleziony
    while (1);                               // Zatrzymuje program jeśli czujnik nie działa
  }

  // Inicjalizacja struktury JSON
  initializeJSON();

  // Konfiguracja serwera HTTP
  server.on("/sensor", HTTP_GET, handleSensorRequest);  // Endpoint dla zapytań o dane z czujników
  server.begin();

  // Konfiguracja klienta WebSocket
  webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");      // Połączenie z serwerem WebSocket
  
  // Obsługa zdarzeń WebSocket - ważne dla synchronizacji po ponownym podłączeniu urządzenia
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
      case WStype_CONNECTED:
        {
          Serial.printf("WebSocket Connected to: %s\n", payload);
          
          // Identyfikuj się jako ESP32 room stats sensor
          webSocketClient.sendTXT("{\"type\":\"esp32_identification\",\"channel\":\"roomStats\"}");
          Serial.println("ESP32 identification sent");
          
          // Połączenie nawiązane po restarcie/ponownym podłączeniu - wysyłamy aktualne dane z czujników
          updateJSONData();
          sendWebSocketData();
          lastSendTime = millis();           // Resetuje timer żeby nie wysłać ponownie od razu
          Serial.println("Initial sensor data sent after reconnection");
          break;
        }    
    }
  });
  webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);    // Automatyczne ponowne łączenie
}

void loop() {
  server.handleClient();                     // Obsługuje przychodzące zapytania HTTP
  webSocketClient.loop();                    // Utrzymuje połączenie WebSocket

  // Sprawdza czy minęło 10 sekund od ostatniego wysłania danych
  if (millis() - lastSendTime >= SENSOR_SEND_INTERVAL) {
    updateJSONData();                        // Aktualizuje dane z czujnika w globalnym JSON
    sendWebSocketData();                     // Wysyła dane przez WebSocket
    lastSendTime = millis();                 // Resetuje timer dla następnego cyklu
  }
}
