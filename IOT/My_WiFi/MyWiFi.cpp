// Lightweight WiFi helper for ESP32 sketches.
// Responsibilities:
//  * Perform an initial blocking connect with a timeout
//  * Expose a simple isConnected() predicate
//  * Provide a non-blocking loop() that periodically retries if disconnected
//
// Design notes:
//  - Initial connect blocks up to 15s to give deterministic bring-up before starting
//    higher-level protocols (e.g., WebSockets, encryption key exchange).
//  - Auto-reconnect is enabled (setAutoReconnect) but an explicit periodic reconnect
//    attempt is also made in loop() as a safety net.
//  - Credentials are currently compiled-in; consider moving to a secure provisioning
//    method (NVS, captive portal) for production deployments.

#include <MyWiFi.h>
#include <Arduino.h>

// NOTE: Plaintext credentials (development). For production, store in NVS or
// load from a secure channel at runtime.
const char* WIFI_SSID     = "PLAY_Swiatlowod_610C";
const char* WIFI_PASSWORD = "HQj1@59PUaaz";

void MyWiFi::connect() {
    WiFi.mode(WIFI_STA);                 // Station mode only (no AP)
    WiFi.setAutoReconnect(true);         // Enable SDK auto reconnect
    WiFi.persistent(false);              // Avoid writing creds repeatedly to flash
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Łączenie z WiFi...");
    unsigned long start = millis();
    // Block (with progress dots) until connected or timeout reached
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Połączono z WiFi!");
        Serial.print("Adres IP: "); Serial.println(WiFi.localIP());
        Serial.print("MAC: ");     Serial.println(WiFi.macAddress());
    } else {
        Serial.println("Nie udało się połączyć z WiFi (timeout). Próba automatycznego ponowienia w tle...");
    }
}

bool MyWiFi::isConnected() {
    return WiFi.status() == WL_CONNECTED; // Simple status check
}

void MyWiFi::loop() {
    static unsigned long lastAttempt = 0;        // Tracks last manual reconnect try
    const unsigned long retryEveryMs = 5000;     // Minimum interval between manual retries
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastAttempt > retryEveryMs) {
            Serial.println("WiFi rozłączone, próba ponownego połączenia...");
            WiFi.reconnect();
            lastAttempt = millis();
        }
    }
}
