// Biblioteki potrzebne do obsługi ESP32 i czujnika BME280
#include <Wire.h>             // Komunikacja I2C z czujnikiem BME280
#include <Adafruit_Sensor.h>  // Podstawowa biblioteka dla czujników Adafruit
#include <Adafruit_BME280.h>  // Obsługa czujnika BME280 (temperatura, wilgotność, ciśnienie)
#include <WiFiManager.h>      // Automatyczne zarządzanie połączeniem WiFi
#include <ArduinoJson.h>      // Tworzenie i parsowanie JSON
#include <WebServer.h>        // Serwer HTTP do obsługi zapytań REST
#include <WiFi.h>             // Podstawowe funkcje WiFi
#include <WebSocketsClient.h> // Klient WebSocket do komunikacji real-time

// Konfiguracja pinów I2C i obiektów
#define SDA_PIN 4                            // Pin SDA dla komunikacji I2C
#define SCL_PIN 5                            // Pin SCL dla komunikacji I2C
WebServer server(80);                        // Serwer HTTP na porcie 80

Adafruit_BME280 bme;                         // Obiekt czujnika BME280 (I2C)

// Obiekty do komunikacji i timing
WebSocketsClient webSocketClient;            // Klient WebSocket do wysyłania danych w czasie rzeczywistym
unsigned long lastSendTime = 0;              // Czas ostatniego wysłania danych przez WebSocket
const unsigned long sendInterval = 10000;   // Interwał wysyłania danych - 10 sekund

// Globalny dokument JSON do przechowywania danych z czujników
StaticJsonDocument<200> JSONpayload;

// Inicjalizuje strukturę JSON z podstawowymi polami dla danych z czujników
void initializeJSON() {
  JSONpayload["channel"] = "roomStats";      // Kanał identyfikujący typ danych (statystyki pomieszczenia)
  JSONpayload["temperature"] = "";           // Temperatura (będzie wypełniona później)
  JSONpayload["humidity"] = "";              // Wilgotność (będzie wypełniona później)
  JSONpayload["pressure"] = "";              // Ciśnienie (będzie wypełnione później)
}

// Aktualizuje globalny JSON z aktualnymi danymi z czujnika BME280
void updateJSONWithSensorData() {
  // Bezpośredni odczyt z czujnika i formatowanie danych z jednostkami
  JSONpayload["temperature"] = String(bme.readTemperature(), 0) + " °C";        // Temperatura w °C (0 miejsc po przecinku)
  JSONpayload["humidity"] = String(bme.readHumidity(), 0) + " %";               // Wilgotność w % (0 miejsc po przecinku)
  JSONpayload["pressure"] = String(bme.readPressure() / 100.0F, 0) + " hPa";    // Ciśnienie w hPa (konwersja z Pa)
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
  serializeJson(JSONpayload, jsonStr);       // Konwertuje globalny JSON na string
  webSocketClient.sendTXT(jsonStr);          // Wysyła przez WebSocket
  Serial.println("Sent: " + jsonStr);       // Debug: wyświetla wysłane dane
}

void setup() {
  Serial.begin(19200);                       // Inicjalizacja komunikacji szeregowej do debugowania
  WiFiManager wm;                            // Manager automatycznego połączenia WiFi
  Wire.begin(SDA_PIN, SCL_PIN);              // Inicjalizacja komunikacji I2C z określonymi pinami
  wm.autoConnect("AutoConnectAP");           // Tworzy punkt dostępu jeśli nie może się połączyć

  Serial.println("Połączono z siecią WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());            // Wyświetla IP ESP32 w sieci
  
  // Inicjalizacja czujnika BME280
  bool status = bme.begin(0x76);             // Próba połączenia z czujnikiem (adres I2C 0x76)
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
  webSocketClient.begin("192.168.1.4", 8886, "/");      // Połączenie z serwerem WebSocket
  
  // Obsługa zdarzeń WebSocket - ważne dla synchronizacji po ponownym podłączeniu urządzenia
  webSocketClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
      case WStype_CONNECTED:
        {
          Serial.printf("WebSocket Connected to: %s\n", payload);
          // Połączenie nawiązane po restarcie/ponownym podłączeniu - wysyłamy aktualne dane z czujników
          updateJSONWithSensorData();
          sendWebSocketData();
          lastSendTime = millis();           // Resetuje timer żeby nie wysłać ponownie od razu
          Serial.println("Initial sensor data sent after reconnection");
          break;
        }    
    }
  });
  webSocketClient.setReconnectInterval(5000);            // Automatyczne ponowne łączenie co 5s
}

void loop() {
  server.handleClient();                     // Obsługuje przychodzące zapytania HTTP
  webSocketClient.loop();                    // Utrzymuje połączenie WebSocket

  // Sprawdza czy minęło 10 sekund od ostatniego wysłania danych
  if (millis() - lastSendTime >= sendInterval) {
    updateJSONWithSensorData();              // Aktualizuje dane z czujnika w globalnym JSON
    sendWebSocketData();                     // Wysyła dane przez WebSocket
    lastSendTime = millis();                 // Resetuje timer dla następnego cyklu
  }
}
