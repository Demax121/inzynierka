#include "MyWiFi.h"
#include <Arduino.h>

const char* WIFI_SSID     = "PLAY_Swiatlowod_610C";
const char* WIFI_PASSWORD = "HQj1@59PUaaz";

void MyWiFi::connect() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (++retry > 60) {
      return;
    }
  }
}
