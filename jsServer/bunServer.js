import { WebSocketServer } from "ws";

const PORT = 3000; // port wewnętrzny kontenera (BUN_API_PORT_NATIVE)

const wss = new WebSocketServer({ port: PORT });

// Mapa klientów z ich typami
const clients = new Map();

wss.on("connection", (ws) => {
  // Domyślnie oznacz jako frontend klient
  clients.set(ws, { type: 'frontend', channel: null });
  
  ws.on("message", (message) => {
    try {
      const data = JSON.parse(message);

      // Sprawdź czy to wiadomość identyfikacyjna od ESP32
      if (data.type === 'esp32_identification') {
        clients.set(ws, { type: 'esp32', channel: data.channel });
        console.log(`ESP32 device identified: ${data.channel}`);
        return;
      }

      // Obsługa kanału "doorStatus"
      if (data.channel === "doorStatus") {
        // Rozsyłaj tylko do frontend klientów i konkretnie ESP32 door sensor
        wss.clients.forEach((client) => {
          if (client.readyState === client.OPEN) {
            const clientInfo = clients.get(client);
            if (clientInfo && (clientInfo.type === 'frontend' || (clientInfo.type === 'esp32' && clientInfo.channel === 'doorStatus'))) {
              client.send(JSON.stringify({
                channel: "doorStatus",
                status: data.status
              }));
            }
          }
        });
      }

      // Obsługa kanału "roomStats"
      if (data.channel === "roomStats") {
        // Rozsyłaj tylko do frontend klientów (ESP32 roomStats tylko wysyła dane)
        wss.clients.forEach((client) => {
          if (client.readyState === client.OPEN) {
            const clientInfo = clients.get(client);
            if (clientInfo && clientInfo.type === 'frontend') {
              client.send(JSON.stringify({
                channel: "roomStats",
                temperature: data.temperature,
                humidity: data.humidity,
                pressure: data.pressure
              }));
            }
          }
        });
      }

      // Obsługa kanału "mainLights"
      if (data.channel === "mainLights" || (typeof data.channel === "undefined" && typeof data.lightStatus === "string")) {
        const lightStatus = data.lightStatus;
        
        // Sprawdź czy status jest poprawny
        if (lightStatus === "ON" || lightStatus === "OFF") {
          // Rozsyłaj do wszystkich klientów OPRÓCZ nadawcy
          wss.clients.forEach((client) => {
            if (client.readyState === client.OPEN && client !== ws) {
              const clientInfo = clients.get(client);
              // Tylko frontend klienty ORAZ konkretnie ESP32 mainLights
              if (clientInfo && (clientInfo.type === 'frontend' || (clientInfo.type === 'esp32' && clientInfo.channel === 'mainLights'))) {
                client.send(JSON.stringify({
                  channel: "mainLights",
                  lightStatus: lightStatus
                }));
              }
            }
          });
          
          console.log(`MainLights state broadcasted: ${lightStatus} (excluding sender)`);
        }
      }
    } catch (err) {
      // Niepoprawny JSON, ignoruj
    }
  });

  // Usuń klienta z mapy gdy się rozłącza
  ws.on('close', () => {
    clients.delete(ws);
  });
});

console.log(`WebSocket server listening on ws://0.0.0.0:${PORT} (doorStatus, roomStats & mainLights channels)`);
