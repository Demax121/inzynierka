#include "MyWebSocket.h"

void MyWebSocket::begin() {
    webSocketClient.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT);
    webSocketClient.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL);
}