#ifndef MY_WIFI_SECURE_H
#define MY_WIFI_SECURE_H

#include <WiFi.h>
#include <WiFiClientSecure.h>

class MyWiFiSecure {
  public:
    static void connect();
    static WiFiClientSecure& getSecureClient();

  private:
    static WiFiClientSecure secureClient;
};

#endif
