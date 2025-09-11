import { WebSocketServer } from "ws";

const PORT = 3000;
const wss = new WebSocketServer({ port: PORT });

const clients = new Map();
let lastRoomTemperature = null;

wss.on("connection", (ws) => {
  const HEARTBEAT_INTERVAL = 10000;

  setInterval(() => {
    wss.clients.forEach((client) => {
      const clientInfo = clients.get(client);
      if (client.readyState === client.OPEN && clientInfo?.type === 'esp32') {
        client.send(JSON.stringify({
          type: "heartbeat",
          channel: clientInfo.channel
        }));
      }
    });
  }, HEARTBEAT_INTERVAL);

  clients.set(ws, { type: 'frontend', channel: null });

  ws.on("message", (message) => {
    try {
      const data = JSON.parse(message);

      if (data.type === 'esp32_identification') {
        clients.set(ws, { type: 'esp32', channel: data.channel });

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
        if (typeof data.temperature === 'number') {
          lastRoomTemperature = data.temperature;
        }

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

        if (typeof data.temperature === 'number') {
          wss.clients.forEach(client => {
            if (client.readyState === client.OPEN) {
              const ci = clients.get(client);
              if (ci && ci.type === 'esp32' && ci.channel === 'klimatyzacja') {
                client.send(JSON.stringify({
                  channel: 'klimatyzacja',
                  temperature: data.temperature
                }));
              }
            }
          });
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
        }
        else if (senderInfo && senderInfo.type === 'frontend' && typeof data.requestedTemp === 'number') {
          const temp = Math.round(data.requestedTemp);
          wss.clients.forEach(client => {
            if (client.readyState === client.OPEN) {
              const ci = clients.get(client);
              if (ci && ci.type === 'esp32' && ci.channel === 'klimatyzacja') {
                client.send(JSON.stringify({
                  channel: 'klimatyzacja',
                  requestedTemp: temp
                }));
              }
            }
          });
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
        }
      }

    } catch (err) {
    }
  });

  ws.on('close', () => {
    const clientInfo = clients.get(ws);
    if (clientInfo && clientInfo.type === 'esp32') {
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
