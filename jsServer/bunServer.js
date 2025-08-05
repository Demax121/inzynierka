import { WebSocketServer } from "ws";

const PORT = 3000; // port wewnętrzny kontenera (BUN_API_PORT_NATIVE)

const wss = new WebSocketServer({ port: PORT });

wss.on("connection", (ws) => {
  ws.on("message", (message) => {
    try {
      const data = JSON.parse(message);

      // Obsługa kanału "doorStatus"
      if (data.channel === "doorStatus") {
        wss.clients.forEach((client) => {
          if (client.readyState === ws.OPEN) {
            client.send(JSON.stringify({
              channel: "doorStatus",
              status: data.status
            }));
          }
        });
      }

      // Obsługa kanału "roomStats"
      if (data.channel === "roomStats") {
        wss.clients.forEach((client) => {
          if (client.readyState === ws.OPEN) {
            client.send(JSON.stringify({
              channel: "roomStats",
              temperature: data.temperature,
              humidity: data.humidity,
              pressure: data.pressure
            }));
          }
        });
      }
    } catch (err) {
      // Niepoprawny JSON, ignoruj
    }
  });
});

console.log(`WebSocket server listening on ws://0.0.0.0:${PORT} (doorStatus & roomStats channels)`);
