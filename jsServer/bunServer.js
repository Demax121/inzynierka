import { WebSocketServer } from "ws";
import * as CryptoJS from "crypto-js";
import fetch from "node-fetch"; // Import fetch for HTTP requests

const database_url = "http://offline_backend_server_caddy_dyplom/"; // Updated to use Docker service name
const encryptionKeyMap = new Map(); // Changed from set to Map for key-value pairs


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

// Function to check if API key exists
function hasApiKey(apiKey) {
  return encryptionKeyMap.has(apiKey);
}

// Load encryption keys on server startup
populateKeyMap();

// Optionally, refresh keys periodically (every 5 minutes)
setInterval(populateKeyMap, 5 * 60 * 1000);

const PORT = 3000;
const wss = new WebSocketServer({ port: PORT });
const PING_INTERVAL = 10000; // 10 seconds interval for pinging devices

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

// Encrypt function using AES-128-CBC (available for future use)
function encrypt(plaintext, apiKey) {
  try {
    const encryptionKey = getEncryptionKey(apiKey);
    if (!encryptionKey) {
      console.error(`Encryption key not found for API key: ${apiKey}`);
      return null;
    }

    // Generate random IV
    const iv = CryptoJS.lib.WordArray.random(16);
    
    // Encrypt using AES-128-CBC
    const encrypted = CryptoJS.AES.encrypt(plaintext, CryptoJS.enc.Hex.parse(encryptionKey), {
      iv: iv,
      mode: CryptoJS.mode.CBC,
      padding: CryptoJS.pad.Pkcs7
    });

    // Combine IV and encrypted data
    const combined = iv.concat(encrypted.ciphertext);
    
    return combined.toString(CryptoJS.enc.Hex).toUpperCase();
  } catch (error) {
    console.error('Encryption error:', error);
    return null;
  }
}

// Decrypt function using AES-128-CBC (available for future use)
function decrypt(ciphertext, apiKey) {
  try {
    const encryptionKey = getEncryptionKey(apiKey);
    if (!encryptionKey) {
      console.error(`Encryption key not found for API key: ${apiKey}`);
      return null;
    }

    // Convert hex string to WordArray
    const combined = CryptoJS.enc.Hex.parse(ciphertext);
    
    // Extract IV (first 16 bytes) and encrypted data (rest)
    const iv = CryptoJS.lib.WordArray.create(combined.words.slice(0, 4)); // 4 words = 16 bytes
    const encrypted = CryptoJS.lib.WordArray.create(combined.words.slice(4));
    
    // Create cipher params object
    const cipherParams = CryptoJS.lib.CipherParams.create({
      ciphertext: encrypted
    });
    
    // Decrypt using AES-128-CBC
    const decrypted = CryptoJS.AES.decrypt(cipherParams, CryptoJS.enc.Hex.parse(encryptionKey), {
      iv: iv,
      mode: CryptoJS.mode.CBC,
      padding: CryptoJS.pad.Pkcs7
    });

    return decrypted.toString(CryptoJS.enc.Utf8);
  } catch (error) {
    console.error('Decryption error:', error);
    return null;
  }
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
        clients.set(ws, { type: "esp32", channel: data.channel });
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

        // Jeśli to air_conditioning i mamy ostatnią temperaturę – wyślij od razu (zachowujemy kompatybilność)
        if (data.channel === "air_conditioning" && lastRoomTemperature !== null) {
          ws.send(
            JSON.stringify({
              channel: "air_conditioning",
              temperature: lastRoomTemperature,
            })
          );
        }
        return;
      }

      // All channels now work with unencrypted data (encryption available for future use)
      if (data.channel === "door_sensor") {
        // Validate the new payload structure
        if (!data.payload || !data.device_api_key) {
          console.log(`Invalid door_sensor format - missing required fields`);
          return;
        }

        // Validate API key exists
        if (!hasApiKey(data.device_api_key)) {
          console.log(`Invalid API key for door_sensor: ${data.device_api_key}`);
          return;
        }

        const doorOpen = data.payload.doorOpen;

        console.log(
          `Received door_sensor data: API=${data.device_api_key}, doorIV=${data.IV}, encryptionKey=${getEncryptionKey(data.device_api_key)}`
        );
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

        if (!data.payload || !data.device_api_key) {
          console.log(`Invalid room_stats format - missing required fields`);
          return;
        }

        const temperature = data.payload.temperature;
        const humidity = data.payload.humidity;
        const pressure = data.payload.pressure;

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
                klimaCount++;
                client.send(
                  JSON.stringify({
                    channel: "air_conditioning",
                    temperature: temperature,
                  })
                );
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
                client.send(
                  JSON.stringify({
                    channel: "main_lights",
                    lightON: lightON,
                  })
                );
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
        else if (data.payload && typeof data.payload.lightON === "boolean") {
          const lightON = data.payload.lightON;
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
        if (!data.payload || !data.device_api_key) {
          console.log(`Invalid lux_sensor format - missing required fields`);
          return;
        }

        const lux = data.payload.lux;
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

        if (data.payload && senderInfo && senderInfo.type === "esp32") {
          const payload = data.payload;
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
                client.send(
                  JSON.stringify({
                    channel: "air_conditioning",
                    payload: payload,
                  })
                );
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

console.log(
  `WebSocket server listening on ws://0.0.0.0:${PORT} (door_sensor, room_stats, main_lights, lux_sensor, air_conditioning channels)`
);
