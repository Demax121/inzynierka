#include "MyWiFiSecure.h"
#include "root_cert.h"
#include <Arduino.h>

// Twoje dane WiFi są w bibliotece:
const char* WIFI_SSID     = "PLAY_Swiatlowod_610C";
const char* WIFI_PASSWORD = "HQj1@59PUaaz";

WiFiClientSecure MyWiFiSecure::secureClient;

void MyWiFiSecure::connect() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retry = 0;

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++retry > 60) {
      Serial.println("\n[MyWiFiSecure] WiFi connection failed!");
      return;
    }
  }
  Serial.println("\n[MyWiFiSecure] WiFi connected!");

  // Ładowanie certyfikatu CA
  if (!secureClient.setCACert(root_ca)) {
    Serial.println("[MyWiFiSecure] Failed to load CA certificate!");
  } else {
    Serial.println("[MyWiFiSecure] Root CA loaded successfully");
  }
}

WiFiClientSecure& MyWiFiSecure::getSecureClient() {
  return secureClient;
}
