#ifndef MY_WIFI_H
#define MY_WIFI_H

#include <WiFi.h>

class MyWiFi {
  public:
    static void connect();
    // Returns true if WiFi is connected
    static bool isConnected();
    // Non-blocking upkeep: try to reconnect periodically if disconnected
    static void loop();
};

#endif
