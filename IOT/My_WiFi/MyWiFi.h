#ifndef MY_WIFI_H
#define MY_WIFI_H

#include <WiFi.h>

// Simple WiFi manager facade.
// Usage pattern:
//   MyWiFi::connect();          // blocking initial attempt (with timeout)
//   loop(): MyWiFi::loop();     // call regularly in main loop
//   if (!MyWiFi::isConnected()) // fallback logic / skip network tasks
//
class MyWiFi {
  public:
    // Attempt initial connection (blocks up to ~15s). Enables auto-reconnect.
    static void connect();
    // Returns true if WiFi is currently connected.
    static bool isConnected();
    // Non-blocking upkeep: periodically triggers reconnect if disconnected.
    static void loop();
};

#endif
