import { WebSocketServer } from "ws";

const PORT = 3000;
const wss = new WebSocketServer({ port: PORT });

const clients = new Map();
let lastRoomTemperature = null; // zapamiętana ostatnia temperatura

wss.on("connection", (ws) => {
  clients.set(ws, { type: 'frontend', channel: null });

  ws.on("message", (message) => {
    try {
      const data = JSON.parse(message);

      if (data.type === 'esp32_identification') {
        clients.set(ws, { type: 'esp32', channel: data.channel });
        console.log(`ESP32 device identified: ${data.channel}`);

        // Jeśli to klimatyzacja i mamy ostatnią temperaturę – wyślij od razu (zachowujemy kompatybilność)
        if (data.channel === 'klimatyzacja' && lastRoomTemperature !== null) {
          ws.send(JSON.stringify({
            channel: 'klimatyzacja',
            temperature: lastRoomTemperature
          }));
        }
        return;
      }

      if (data.channel === "doorStatus") {
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

      if (data.channel === "roomStats") {
        console.log(`Received roomStats data:`, data); // Debug log
        
        // zapamiętaj temperaturę
        if (typeof data.temperature === 'number') {
          lastRoomTemperature = data.temperature;
          console.log(`Updated lastRoomTemperature: ${lastRoomTemperature}`);
        } else {
          console.log(`Temperature is not a number: ${typeof data.temperature}, value: ${data.temperature}`);
        }

        // Do frontendów pełne dane (surowe liczby)
        let frontendCount = 0;
        wss.clients.forEach((client) => {
          if (client.readyState === client.OPEN) {
            const clientInfo = clients.get(client);
            if (clientInfo && clientInfo.type === 'frontend') {
              frontendCount++;
              client.send(JSON.stringify({
                channel: "roomStats",
                temperature: data.temperature,
                humidity: data.humidity,
                pressure: data.pressure
              }));
            }
          }
        });
        console.log(`RoomStats broadcasted to ${frontendCount} frontend clients`);

        // Dodatkowo: wyślij samą temperaturę do ESP32 klimatyzacja
        if (typeof data.temperature === 'number') {
            let klimaCount = 0;
            wss.clients.forEach(client => {
              if (client.readyState === client.OPEN) {
                const ci = clients.get(client);
                if (ci && ci.type === 'esp32' && ci.channel === 'klimatyzacja') {
                  klimaCount++;
                  client.send(JSON.stringify({
                    channel: 'klimatyzacja', // Zmieniono z 'roomStats' na 'klimatyzacja'
                    temperature: data.temperature
                  }));
                }
              }
            });
            console.log(`Temperature sent to ${klimaCount} klimatyzacja ESP32 devices`);
        }
      }

      if (data.channel === "mainLights" || (typeof data.channel === "undefined" && typeof data.lightStatus === "string")) {
        const lightStatus = data.lightStatus;
        if (lightStatus === "ON" || lightStatus === "OFF") {
          wss.clients.forEach((client) => {
            if (client.readyState === client.OPEN && client !== ws) {
              const clientInfo = clients.get(client);
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

      if (data.channel === "luxSensor") {
        const lux = data.lux;
        if (typeof lux === 'number') {
          wss.clients.forEach((client) => {
            if (client.readyState === client.OPEN) {
              const clientInfo = clients.get(client);
              if (clientInfo && clientInfo.type === 'frontend') {
                client.send(JSON.stringify({
                  channel: "luxSensor",
                  lux: lux
                }));
              }
            }
          });
        }
      }

      if (data.channel === 'klimatyzacja') {
        const senderInfo = clients.get(ws);

        if (senderInfo && senderInfo.type === 'frontend' && typeof data.klimaStatus === 'string') {
          const status = data.klimaStatus === 'OFF' ? 'OFF' : 'ON';
          wss.clients.forEach(client => {
            if (client.readyState === client.OPEN) {
              const ci = clients.get(client);
              if (ci && (ci.type === 'frontend' || (ci.type === 'esp32' && ci.channel === 'klimatyzacja'))) {
                client.send(JSON.stringify({
                  channel: 'klimatyzacja',
                  klimaStatus: status,
                  temperature: lastRoomTemperature
                }));
              }
            }
          });
          console.log(`Klimatyzacja broadcast (frontend command) -> ${status}`);
        }
        // Nowa sekcja dla requestedTemp
        else if (senderInfo && senderInfo.type === 'frontend' && typeof data.requestedTemp === 'number') {
          const temp = Math.round(data.requestedTemp);
          console.log(`Processing requestedTemp: ${temp} from frontend`); // Debug log
          let sentCount = 0;
          wss.clients.forEach(client => {
            if (client.readyState === client.OPEN) {
              const ci = clients.get(client);
              if (ci && ci.type === 'esp32' && ci.channel === 'klimatyzacja') {
                sentCount++;
                client.send(JSON.stringify({
                  channel: 'klimatyzacja',
                  requestedTemp: temp
                }));
              }
            }
          });
          console.log(`Requested temperature sent to ${sentCount} klimatyzacja ESP32 devices`);
        }
        else if (senderInfo && senderInfo.type === 'esp32' && senderInfo.channel === 'klimatyzacja' && typeof data.klimaStatus !== 'undefined') {
          const status = data.klimaStatus === 'ON' ? 'ON' : 'OFF';
          const currentFunction = data.currentFunction || '';
          wss.clients.forEach(client => {
            if (client.readyState === client.OPEN) {
              const ci = clients.get(client);
              if (ci && ci.type === 'frontend') {
                client.send(JSON.stringify({
                  channel: 'klimatyzacja',
                  klimaStatus: status,
                  currentFunction: currentFunction,
                  temperature: lastRoomTemperature
                }));
              }
            }
          });
          console.log(`Klimatyzacja status od ESP32 -> ${status}, function: ${currentFunction}`);
        }
      }

    } catch (err) {
      // Ignorujemy błędny JSON
    }
  });

  ws.on('close', () => {
    const clientInfo = clients.get(ws);
    if (clientInfo && clientInfo.type === 'esp32') {
      console.log(`ESP32 device disconnected: ${clientInfo.channel}`);
      // Powiadom frontendy o rozłączeniu ESP32
      wss.clients.forEach((client) => {
        if (client.readyState === client.OPEN) {
          const ci = clients.get(client);
          if (ci && ci.type === 'frontend') {
            client.send(JSON.stringify({
              channel: clientInfo.channel,
              status: 'disconnected'
            }));
          }
        }
      });
    }
    clients.delete(ws);
  });
});

console.log(`WebSocket server listening on ws://0.0.0.0:${PORT} (doorStatus, roomStats, mainLights, luxSensor, klimatyzacja channels)`);
