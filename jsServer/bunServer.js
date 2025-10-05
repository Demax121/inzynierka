import { WebSocketServer } from "ws";
import fetch from "node-fetch"; // HTTP requests to PHP backend
import { randomBytes, createCipheriv, createDecipheriv } from 'crypto'; // Node crypto for AES-128-GCM

// Backend PHP base URL (internal Docker network). Can be overridden with BACKEND_INTERNAL_URL env var.
const database_url = process.env.BACKEND_INTERNAL_URL || "http://offline_backend_server_caddy_dyplom/"; // fallback to service name
const encryptionKeyMap = new Map(); // api_key -> encryption_key


function updateLastSeen(apiKey) {
  fetch(`${database_url}updateDeviceStatus.php`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      device_api_key: apiKey,
    }),
  })
    .then((response) => {
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      return response.json();
    })
    .then((data) => {
      console.log("Device last seen updated successfully:", data);
    })
    .catch((error) => {
      console.error("Error updating device last seen:", error);
    });
}



function populateKeyMap() {
  fetch(`${database_url}getEncryptionKeys.php`, {
    method: "GET",
    headers: {
      "Content-Type": "application/json",
    },
  })
    .then((response) => {
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      return response.json();
    })
    .then((data) => {
      // Clear existing entries
      encryptionKeyMap.clear();

      // Populate the Map with API key as key and encryption key as value
      data.data.forEach((device) => {
        encryptionKeyMap.set(device.device_api_key, device.device_encryption_key);
      });

      console.log(`Loaded ${encryptionKeyMap.size} encryption keys`);
    })
    .catch((error) => {
      console.error("Error loading encryption keys:", error);
    });
}

// Function to get encryption key by API key
function getEncryptionKey(apiKey) {
  return encryptionKeyMap.get(apiKey);
}
// Load encryption keys on server startup
populateKeyMap();

// Optionally, refresh keys periodically (every 5 minutes)
setInterval(populateKeyMap, 5 * 60 * 1000);



// AES-128-GCM helpers (nonce 12 bytes, tag 16 bytes). Payload is UTF-8 JSON string.
function encryptPayloadForDevice(obj, apiKey) {
  const key = getEncryptionKey(apiKey);
  if (!key) return null;
  try {
    const nonce = randomBytes(12); // 96-bit recommended
    const cipher = createCipheriv('aes-128-gcm', Buffer.from(key.slice(0,16), 'utf8'), nonce);
    const plain = JSON.stringify(obj);
    const enc = Buffer.concat([cipher.update(plain, 'utf8'), cipher.final()]);
    const tag = cipher.getAuthTag();
    return {
      nonce: nonce.toString('hex'),
      payload: enc.toString('hex'),
      tag: tag.toString('hex'),
      alg: 'AES-128-GCM'
    };
  } catch (e) {
    console.error('Encryption error', e);
    return null;
  }
}

function decryptDevicePayload(data) {
  // Expects: { device_api_key, nonce, payload, tag }
  if (!data || typeof data.payload !== 'string' || typeof data.nonce !== 'string' || typeof data.tag !== 'string') return null;
  const key = getEncryptionKey(data.device_api_key);
  if (!key) return null;
  try {
    const nonce = Buffer.from(data.nonce, 'hex');
    const cipherBuf = Buffer.from(data.payload, 'hex');
    const tag = Buffer.from(data.tag, 'hex');
    if (nonce.length !== 12 || tag.length !== 16) return null;
    const decipher = createDecipheriv('aes-128-gcm', Buffer.from(key.slice(0,16), 'utf8'), nonce);
    decipher.setAuthTag(tag);
    const dec = Buffer.concat([decipher.update(cipherBuf), decipher.final()]);
    return JSON.parse(dec.toString('utf8'));
  } catch (e) {
    console.error('Failed to decrypt/parse device payload', e);
    return null;
  }
}







const PORT = 3000;
const wss = new WebSocketServer({ port: PORT });
const PING_INTERVAL = 10000; // 10 seconds interval for pinging devices

// clients: ws -> { type: 'frontend'|'esp32', channel: string|null, device_api_key?: string }
const clients = new Map();
let lastRoomTemperature = null; // zapamiętana ostatnia temperatura
let pingInterval;

// Start ping interval function
function startPingInterval() {
  // Clear any existing interval
  if (pingInterval) {
    clearInterval(pingInterval);
  }

  // Create new interval to ping all ESP32 devices
  pingInterval = setInterval(() => {
    let pingCount = 0;
    wss.clients.forEach((client) => {
      if (client.readyState === client.OPEN) {
        const clientInfo = clients.get(client);
        if (clientInfo && clientInfo.type === "esp32") {
          client.send(
            JSON.stringify({
              type: "ping",
              channel: clientInfo.channel,
            })
          );
          pingCount++;
        }
      }
    });
    if (pingCount > 0) {
      console.log(`Sent ping to ${pingCount} ESP32 devices`);
    }
  }, PING_INTERVAL);
}


wss.on("connection", (ws) => {
  clients.set(ws, { type: "frontend", channel: null });

  ws.on("message", (message) => {
    let messageStr = message.toString();
    let data;
    
    try {
      // Parse as JSON (encryption/decryption functions available for future use)
      data = JSON.parse(messageStr);
    } catch (parseErr) {
      console.error("Failed to parse message as JSON:", parseErr);
      return;
    }

    try {
      if (data.type === "esp32_identification") {
        // Devices should include device_api_key here for downstream encryption
        clients.set(ws, { type: "esp32", channel: data.channel, device_api_key: data.device_api_key || null });
        console.log(`ESP32 device identified: ${data.channel}`);

        // Send immediate ping to newly connected ESP32 device
        if (ws.readyState === ws.OPEN) {
          ws.send(
            JSON.stringify({
              type: "ping",
              channel: data.channel,
            })
          );
          console.log(`Sent initial ping to newly connected ESP32 device: ${data.channel}`);
        }

        // If it's air_conditioning and we have last temp – send encrypted immediately (GCM)
        if (data.channel === "air_conditioning" && lastRoomTemperature !== null) {
          const apiKey = data.device_api_key || clients.get(ws)?.device_api_key;
          if (apiKey) {
            const enc = encryptPayloadForDevice({ temperature: lastRoomTemperature }, apiKey);
            if (enc) {
              ws.send(
                JSON.stringify({
                  channel: "air_conditioning",
                  ...enc,
                })
              );
            }
          }
        }
        return;
      }

      // All channels from devices now carry encrypted payload: { nonce, payload, tag }
      if (data.channel === "door_sensor") {
        if (!data.payload || !data.device_api_key || !data.nonce || !data.tag) {
          console.log(`Invalid door_sensor format - missing required fields`);
          return;
        }
        const dec = decryptDevicePayload(data);
        if (!dec || typeof dec.doorOpen !== "boolean") return;
        const doorOpen = dec.doorOpen;

        console.log(`Received door_sensor data from API=${data.device_api_key}`);
        updateLastSeen(data.device_api_key);
        
        wss.clients.forEach((client) => {
          if (
            client.readyState === client.OPEN &&
            (clients.get(client)?.type === "frontend" ||
              (clients.get(client)?.type === "esp32" &&
                clients.get(client)?.channel === "door_sensor"))
          ) {
            client.send(
              JSON.stringify({
                channel: "door_sensor",
                device_api_key: data.device_api_key,
                payload: {
                  doorOpen: doorOpen,
                },
              })
            );
          }
        });
      }

      if (data.channel === "room_stats") {
        console.log(`Received room_stats data:`, data);
        if (!data.payload || !data.device_api_key || !data.nonce || !data.tag) {
          console.log(`Invalid room_stats format - missing required fields`);
          return;
        }
        const dec = decryptDevicePayload(data);
        if (!dec) return;
        const temperature = dec.temperature;
        const humidity = dec.humidity;
        const pressure = dec.pressure;

        if (typeof temperature === "number") {
          lastRoomTemperature = temperature;
          console.log(`Updated lastRoomTemperature: ${lastRoomTemperature}`);
        } else {
          console.log(
            `Temperature is not a number: ${typeof temperature}, value: ${temperature}`
          );
        }
        updateLastSeen(data.device_api_key);

        let frontendCount = 0;
        wss.clients.forEach((client) => {
          if (client.readyState === client.OPEN) {
            const clientInfo = clients.get(client);
            if (clientInfo && clientInfo.type === "frontend") {
              frontendCount++;
              client.send(
                JSON.stringify({
                  channel: "room_stats",
                  temperature: temperature,
                  humidity: humidity,
                  pressure: pressure,
                })
              );
            }
          }
        });
        console.log(`room_stats broadcasted to ${frontendCount} frontend clients`);

        if (typeof temperature === "number") {
          let klimaCount = 0;
          wss.clients.forEach((client) => {
            if (client.readyState === client.OPEN) {
              const ci = clients.get(client);
              if (ci && ci.type === "esp32" && ci.channel === "air_conditioning") {
                // Encrypt temp push for this specific device using its api key
                if (ci.device_api_key) {
                  const enc = encryptPayloadForDevice({ temperature: temperature }, ci.device_api_key);
                  if (enc) {
                    klimaCount++;
                    client.send(
                      JSON.stringify({
                        channel: "air_conditioning",
                        ...enc,
                      })
                    );
                  }
                }
              }
            }
          });
          console.log(
            `Temperature sent to ${klimaCount} air_conditioning ESP32 devices`
          );
        }
      }

      if (data.channel === "main_lights") {
        const senderInfo = clients.get(ws);

        // Only require device_api_key for ESP32 messages, not frontend
        if (senderInfo && senderInfo.type === "esp32" && !data.device_api_key) {
          console.log(`Invalid main_lights format - missing device_api_key from ESP32`);
          return;
        }

        // Handle frontend command (no device_api_key required)
        if (
          senderInfo &&
          senderInfo.type === "frontend" &&
          typeof data.lightON === "boolean"
        ) {
          const lightON = data.lightON;
          // Don't call updateLastSeen for frontend messages since there's no device_api_key
          
          wss.clients.forEach((client) => {
            if (client.readyState === client.OPEN && client !== ws) {
              const clientInfo = clients.get(client);
              if (clientInfo && clientInfo.type === "esp32") {
                // Encrypt for each device using its api key (GCM)
                if (clientInfo.device_api_key) {
                  const enc = encryptPayloadForDevice({ lightON }, clientInfo.device_api_key);
                  if (enc) {
                    client.send(
                      JSON.stringify({
                        channel: "main_lights",
                        ...enc,
                      })
                    );
                  }
                }
              } else if (clientInfo && clientInfo.type === "frontend") {
                client.send(
                  JSON.stringify({
                    channel: "main_lights",
                    lightON: lightON,
                  })
                );
              }
            }
          });
          console.log(`main_lights command from frontend: ${lightON ? "ON" : "OFF"}`);
        }
        // Handle ESP32 status update (requires device_api_key)
        else if (data.payload && data.nonce && data.tag) {
          const dec = decryptDevicePayload(data);
          if (!dec || typeof dec.lightON !== "boolean") return;
          const lightON = dec.lightON;
          // Save device_api_key on the connection if missing
          const ci = clients.get(ws) || {};
          if (!ci.device_api_key && data.device_api_key) {
            clients.set(ws, { ...ci, device_api_key: data.device_api_key });
          }
          updateLastSeen(data.device_api_key);
          wss.clients.forEach((client) => {
            if (client.readyState === client.OPEN && client !== ws) {
              const clientInfo = clients.get(client);
              if (clientInfo && clientInfo.type === "frontend") {
                client.send(
                  JSON.stringify({
                    channel: "main_lights",
                    lightON: lightON,
                  })
                );
              }
            }
          });
          console.log(`main_lights status from ESP32: ${lightON ? "ON" : "OFF"}`);
        } else {
          console.log(`Invalid main_lights format - missing valid data`);
        }
      }

      if (data.channel === "lux_sensor") {
        if (!data.payload || !data.device_api_key || !data.nonce || !data.tag) {
          console.log(`Invalid lux_sensor format - missing required fields`);
          return;
        }
        const dec = decryptDevicePayload(data);
        if (!dec) return;
        const lux = dec.lux;
        if (typeof lux === "number") {
          updateLastSeen(data.device_api_key);
          wss.clients.forEach((client) => {
            if (client.readyState === client.OPEN) {
              const clientInfo = clients.get(client);
              if (clientInfo && clientInfo.type === "frontend") {
                client.send(
                  JSON.stringify({
                    channel: "lux_sensor",
                    lux: lux,
                  })
                );
              }
            }
          });
          console.log(`Lux amount broadcasted: ${lux} (excluding sender)`);
        }
      }

      if (data.channel === "air_conditioning") {
        const senderInfo = clients.get(ws);

        if (senderInfo && senderInfo.type === "esp32" && !data.device_api_key) {
          console.log(`Invalid air_conditioning format - missing device_api_key from ESP32`);
          return;
        }

        if (data.payload && data.nonce && data.tag && senderInfo && senderInfo.type === "esp32") {
          const dec = decryptDevicePayload(data);
          if (!dec) return;
          const payload = dec;
          // ensure we know device_api_key for this connection
          const ci = clients.get(ws) || {};
          if (!ci.device_api_key && data.device_api_key) {
            clients.set(ws, { ...ci, device_api_key: data.device_api_key });
          }
          updateLastSeen(data.device_api_key);

          wss.clients.forEach((client) => {
            if (client.readyState === client.OPEN && client !== ws) {
              const ci = clients.get(client);
              if (ci && ci.type === "frontend") {
                client.send(
                  JSON.stringify({
                    channel: "air_conditioning",
                    payload: {
                      requestedTemp: payload.requestedTemp,
                      function: payload.function,
                      klimaON: payload.klimaON,
                      manualOverride: payload.manualOverride,
                      currentTemp: lastRoomTemperature,
                    },
                  })
                );
              }
            }
          });
          console.log(
            `air_conditioning status from ESP32 -> klimaON: ${payload.klimaON}, function: ${payload.function}, manualOverride: ${payload.manualOverride}`
          );
        }
        else if (data.payload && senderInfo && senderInfo.type === "frontend") {
          const payload = data.payload;

          wss.clients.forEach((client) => {
            if (client.readyState === client.OPEN && client !== ws) {
              const ci = clients.get(client);
              if (ci && ci.type === "esp32" && ci.channel === "air_conditioning") {
                if (ci.device_api_key) {
                  const enc = encryptPayloadForDevice(payload, ci.device_api_key);
                  if (enc) {
                    client.send(
                      JSON.stringify({
                        channel: "air_conditioning",
                        ...enc,
                      })
                    );
                  }
                }
              }
            }
          });
          console.log(
            `air_conditioning command from frontend -> klimaON: ${payload.klimaON}, manualOverride: ${payload.manualOverride}`
          );
        }
      }
    } catch (err) {
      console.error("Error processing message:", err);
    }
  });

  ws.on("close", () => {
    const clientInfo = clients.get(ws);
    if (clientInfo && clientInfo.type === "esp32") {
      console.log(`ESP32 device disconnected: ${clientInfo.channel}`);
      wss.clients.forEach((client) => {
        if (client.readyState === client.OPEN) {
          const ci = clients.get(client);
          if (ci && ci.type === "frontend") {
            client.send(
              JSON.stringify({
                channel: clientInfo.channel,
                status: "disconnected",
              })
            );
          }
        }
      });
    }
    clients.delete(ws);
  });
});

// Start the ping interval when server initializes
startPingInterval();

// Clean up interval when process exits
process.on("SIGINT", () => {
  if (pingInterval) {
    clearInterval(pingInterval);
  }
  process.exit();
});


console.log(
  `WebSocket server listening on ws://0.0.0.0:${PORT} (door_sensor, room_stats, main_lights, lux_sensor, air_conditioning channels)`
);
