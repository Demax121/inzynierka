// Plik: IOT/pliki/My_WiFi/MyWiFi.h
#ifndef MY_WIFI_H
#define MY_WIFI_H

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

class MyWiFi {
  public:
    static void connect();
    static const char* getCert(); // Dodano funkcję do pobierania certyfikatu
};

#endif