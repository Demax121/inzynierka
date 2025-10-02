#include <MyWiFi.h>
#include <Arduino.h>

const char* WIFI_SSID     = "PLAY_Swiatlowod_610C";
const char* WIFI_PASSWORD = "HQj1@59PUaaz";

void MyWiFi::connect() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Łączenie z WiFi...");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Połączono z WiFi!");
        Serial.print("Adres IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("MAC: ");
        Serial.println(WiFi.macAddress());
    } else {
        Serial.println("Nie udało się połączyć z WiFi (timeout). Próba automatycznego ponowienia w tle...");
    }
}

bool MyWiFi::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void MyWiFi::loop() {
    static unsigned long lastAttempt = 0;
    const unsigned long retryEveryMs = 5000;
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastAttempt > retryEveryMs) {
            Serial.println("WiFi rozłączone, próba ponownego połączenia...");
            WiFi.reconnect();
            lastAttempt = millis();
        }
    }
}
