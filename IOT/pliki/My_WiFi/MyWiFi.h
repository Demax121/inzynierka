 #ifndef MY_WIFIV2_H
 #define MY_WIFIV2_H

#include <WiFi.h>
#include <WiFiClientSecure.h>

class MyWiFiv2 {
  public:
  static void connect();
  static const char* getCert(); // Dodano funkcję do pobierania certyfikatu
};

#endif