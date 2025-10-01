#include "MyWiFi.h"
#include <Arduino.h>

const char* WIFI_SSID     = "Praca_dyplomowa";
const char* WIFI_PASSWORD = "AJP@INZYNIERKA2137";

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
