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

        // Jeśli to air_conditioning i mamy ostatnią temperaturę – wyślij od razu (zachowujemy kompatybilność)
        if (data.channel === 'air_conditioning' && lastRoomTemperature !== null) {
          ws.send(JSON.stringify({
            channel: 'air_conditioning',
            temperature: lastRoomTemperature
          }));
        }
        return;
      }

      if (data.channel === "door_sensor") {
        // Obsługa tylko nowego formatu z payload
        if (!data.payload) {
          console.log(`Invalid door_sensor format - missing payload object`);
          return;
        }
        
        const doorOpen = data.payload.doorOpen;
        
        wss.clients.forEach((client) => {
          if (client.readyState === client.OPEN) {
            const clientInfo = clients.get(client);
            if (clientInfo && (clientInfo.type === 'frontend' || (clientInfo.type === 'esp32' && clientInfo.channel === 'door_sensor'))) {
              client.send(JSON.stringify({
                channel: "door_sensor",
                doorOpen: doorOpen
              }));
            }
          }
        });
      }

      if (data.channel === "room_stats") {
        console.log(`Received room_stats data:`, data); // Debug log
        
        // Obsługa tylko nowego formatu z payload
        if (!data.payload) {
          console.log(`Invalid room_stats format - missing payload object`);
          return;
        }
        
        const temperature = data.payload.temperature;
        const humidity = data.payload.humidity;
        const pressure = data.payload.pressure;
        
        // zapamiętaj temperaturę
        if (typeof temperature === 'number') {
          lastRoomTemperature = temperature;
          console.log(`Updated lastRoomTemperature: ${lastRoomTemperature}`);
        } else {
          console.log(`Temperature is not a number: ${typeof temperature}, value: ${temperature}`);
        }

        // Do frontendów pełne dane (surowe liczby)
        let frontendCount = 0;
        wss.clients.forEach((client) => {
          if (client.readyState === client.OPEN) {
            const clientInfo = clients.get(client);
            if (clientInfo && clientInfo.type === 'frontend') {
              frontendCount++;
              client.send(JSON.stringify({
                channel: "room_stats",
                temperature: temperature,
                humidity: humidity,
                pressure: pressure
              }));
            }
          }
        });
        console.log(`room_stats broadcasted to ${frontendCount} frontend clients`);

        // Dodatkowo: wyślij samą temperaturę do ESP32 air_conditioning
        if (typeof temperature === 'number') {
            let klimaCount = 0;
            wss.clients.forEach(client => {
              if (client.readyState === client.OPEN) {
                const ci = clients.get(client);
                if (ci && ci.type === 'esp32' && ci.channel === 'air_conditioning') {
                  klimaCount++;
                  client.send(JSON.stringify({
                    channel: 'air_conditioning',
                    temperature: temperature
                  }));
                }
              }
            });
            console.log(`Temperature sent to ${klimaCount} air_conditioning ESP32 devices`);
        }
      }

      if (data.channel === "main_lights" || (typeof data.channel === "undefined" && typeof data.lightStatus === "string")) {
        const lightStatus = data.lightStatus;
        if (lightStatus === "ON" || lightStatus === "OFF") {
          wss.clients.forEach((client) => {
            if (client.readyState === client.OPEN && client !== ws) {
              const clientInfo = clients.get(client);
              if (clientInfo && (clientInfo.type === 'frontend' || (clientInfo.type === 'esp32' && clientInfo.channel === 'main_lights'))) {
                client.send(JSON.stringify({
                  channel: "main_lights",
                  lightStatus: lightStatus
                }));
              }
            }
          });
          console.log(`main_lights state broadcasted: ${lightStatus} (excluding sender)`);
        }
      }

      if (data.channel === "lux_sensor") {
        // Obsługa tylko nowego formatu z payload
        if (!data.payload) {
          console.log(`Invalid lux_sensor format - missing payload object`);
          return;
        }
        
        const lux = data.payload.lux;
        if (typeof lux === 'number') {
          wss.clients.forEach((client) => {
            if (client.readyState === client.OPEN) {
              const clientInfo = clients.get(client);
              if (clientInfo && clientInfo.type === 'frontend') {
                client.send(JSON.stringify({
                  channel: "lux_sensor",
                  lux: lux
                }));
              }
            }
          });
          console.log(`Lux amount broadcasted: ${lux} (excluding sender)`);
        }
      }

      if (data.channel === 'air_conditioning') {
        const senderInfo = clients.get(ws);

        if (senderInfo && senderInfo.type === 'frontend' && typeof data.klimaStatus === 'string') {
          const status = data.klimaStatus === 'OFF' ? 'OFF' : 'ON';
          wss.clients.forEach(client => {
            if (client.readyState === client.OPEN) {
              const ci = clients.get(client);
              if (ci && (ci.type === 'frontend' || (ci.type === 'esp32' && ci.channel === 'air_conditioning'))) {
                client.send(JSON.stringify({
                  channel: 'air_conditioning',
                  klimaStatus: status,
                  temperature: lastRoomTemperature
                }));
              }
            }
          });
          console.log(`air_conditioning broadcast (frontend command) -> ${status}`);
        }
        // Nowa sekcja dla requestedTemp
        else if (senderInfo && senderInfo.type === 'frontend' && typeof data.requestedTemp === 'number') {
          const temp = Math.round(data.requestedTemp);
          console.log(`Processing requestedTemp: ${temp} from frontend`); // Debug log
          let sentCount = 0;
          wss.clients.forEach(client => {
            if (client.readyState === client.OPEN) {
              const ci = clients.get(client);
              if (ci && ci.type === 'esp32' && ci.channel === 'air_conditioning') {
                sentCount++;
                client.send(JSON.stringify({
                  channel: 'air_conditioning',
                  requestedTemp: temp
                }));
              }
            }
          });
          console.log(`Requested temperature sent to ${sentCount} air_conditioning ESP32 devices`);
        }
        else if (senderInfo && senderInfo.type === 'esp32' && senderInfo.channel === 'air_conditioning' && typeof data.klimaStatus !== 'undefined') {
          const status = data.klimaStatus === 'ON' ? 'ON' : 'OFF';
          const currentFunction = data.currentFunction || '';
          wss.clients.forEach(client => {
            if (client.readyState === client.OPEN) {
              const ci = clients.get(client);
              if (ci && ci.type === 'frontend') {
                client.send(JSON.stringify({
                  channel: 'air_conditioning',
                  klimaStatus: status,
                  currentFunction: currentFunction,
                  temperature: lastRoomTemperature
                }));
              }
            }
          });
          console.log(`air_conditioning status od ESP32 -> ${status}, function: ${currentFunction}`);
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

console.log(`WebSocket server listening on ws://0.0.0.0:${PORT} (door_sensor, room_stats, main_lights, lux_sensor, air_conditioning channels)`);
