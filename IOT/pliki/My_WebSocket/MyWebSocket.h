#ifndef MYWEBSOCKET_H
#define MYWEBSOCKET_H

#include <WebSocketsClient.h>

class MyWebSocket {
private:
    static const char* WEBSOCKET_SERVER;
    static const int WEBSOCKET_PORT;
    static const unsigned long WEBSOCKET_RECONNECT_INTERVAL;
    static WebSocketsClient webSocketClient;

public:
    static void StartSocket();
    static void Loop();
    static void SendTXT(const char* data, size_t len);
    static bool isConnected();
};

#endif