#include <MyWiFi.h>
#include <Arduino.h>

const char* WIFI_SSID     = "PLAY_Swiatlowod_610C";
const char* WIFI_PASSWORD = "HQj1@59PUaaz";

void MyWiFi::connect() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Łączenie z WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Połączono z WiFi!");
    Serial.print("Adres IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
}
