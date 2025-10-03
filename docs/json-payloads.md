# JSON payload reference

This document captures the concrete JSON shapes used across ESP32 devices, Bun WebSocket server, Vue frontend, and PHP endpoints.

Encryption overview
- Device ↔ Server payloads are encrypted with AES-128-CBC (PKCS7).
- Key: per-device `device_encryption_key` (first 16 bytes used as AES key).
- IV: 16 random bytes per message; hex-encoded as `msgIV`.
- Ciphertext: hex-encoded in `payload` field.
- Frontend broadcasts remain plaintext JSON (unchanged).

## WebSocket channels (device → server → frontend)

All ESP32-origin messages include:
- Identification message (device → server):
  { "type": "esp32_identification", "channel": "<channel>", "device_api_key": "<apiKey>" }
  The server stores device_api_key per connection to encrypt replies.
- identity: string (device label)
- channel: string (one of: door_sensor, room_stats, main_lights, lux_sensor, air_conditioning)
- device_api_key: string
- msgIV: string (32 hex chars: 16-byte IV)
- payload: string (hex ciphertext of a UTF‑8 JSON object; see per-channel bodies below)

### door_sensor
- Device → Server:
  {
    "identity": "main_door_sensor",
    "channel": "door_sensor",
    "device_api_key": "<apiKey>",
    "msgIV": "<32-hex>",
    "payload": "<hex-cipher>"    // decrypts to { "doorOpen": boolean }
  }
- Server → Frontends: identical shape (broadcast)

### room_stats
- Device → Server:
  {
    "identity": "room_stats_sensor",
    "channel": "room_stats",
    "device_api_key": "<apiKey>",
    "msgIV": "<32-hex>",
    "payload": "<hex-cipher>"     // decrypts to { "temperature": number, "humidity": number, "pressure": number }
  }
- Server → Frontends:
  { "channel": "room_stats", "temperature": number, "humidity": number, "pressure": number }
- Server → ESP32 (air_conditioning devices):
  { "channel": "air_conditioning", "temperature": number }

### lux_sensor
- Device → Server:
  {
    "identity": "lux_sensor",
    "channel": "lux_sensor",
    "device_api_key": "<apiKey>",
    "msgIV": "<32-hex>",
    "payload": "<hex-cipher>"     // decrypts to { "lux": number }
  }
- Server → Frontends:
  { "channel": "lux_sensor", "lux": number }

### main_lights
- Frontend → Server (command):
  { "channel": "main_lights", "lightON": boolean }
- Device → Server (status):
  {
    "identity": "lights_controller",
    "channel": "main_lights",
    "device_api_key": "<apiKey>",
    "msgIV": "<32-hex>",
    "payload": "<hex-cipher>"     // decrypts to { "lightON": boolean }
  }
- Server → Frontends (mirror status):
  { "channel": "main_lights", "lightON": boolean }
- Server → ESP32 (relay command):
  { "channel": "main_lights", "msgIV": "<32-hex>", "payload": "<hex-cipher>" } // decrypts to { "lightON": boolean }

### air_conditioning
- Device → Server (status):
  {
    "identity": "air_conditioning",
    "channel": "air_conditioning",
    "device_api_key": "<apiKey>",
    "msgIV": "<32-hex>",
    "payload": "<hex-cipher>"     // decrypts to { requestedTemp, function, klimaON, manualOverride }
  }
- Frontend → Server (command to device):
  { "channel": "air_conditioning", "payload": { "requestedTemp"?: number, "function"?: string, "klimaON"?: boolean, "manualOverride"?: boolean } }
- Server → Frontends (status broadcast):
  { "channel": "air_conditioning", "payload": { "requestedTemp": number, "function": string, "klimaON": boolean, "manualOverride": boolean, "currentTemp": number|null } }
- Server → ESP32 (room temp from room_stats):
  { "channel": "air_conditioning", "msgIV": "<32-hex>", "payload": "<hex-cipher>" } // decrypts to { "temperature": number }

## PHP endpoints (request/response)

Base: http://offline_backend_server_caddy_dyplom/<file>.php (inside Docker network). From frontend, use `linkStore.getPhpApiUrl(name)`.

- saveRoomStats.php (POST)
  Req: { "temperature": number, "humidity": number, "pressure": number }
  Res: { "success": true } | 400/500 with { "success": false, "error": string }

- getDoorStatus.php (GET)
  Res: { "success": true, "data": [ { "status_id": number, "status_date": string, "status_name": "Door open"|"Door closed" } ], "count": number }

- saveDoorStatus.php (POST)
  Req: { "status": "Door open"|"Door closed" }
  Res: { "success": true } | 400/500 with { "success": false, "error": string }

- getBlindsConfig.php (GET)
  Res: { "min_lux": number, "max_lux": number, "automate": boolean }

- saveBlindsConfig.php (POST)
  Req: { "min_lux": number, "max_lux": number, "automate": boolean }
  Res: { "success": true, "min_lux": number, "max_lux": number, "automate": boolean } | { "success": false, "error": string }

- setBlindsAutomate.php (POST)
  Req: { "automate": boolean }
  Res: { "success": true, "automate": boolean } | { "success": false, "error": string }

- chooseProfile.php (GET)
  Query: ?id=<number> | ?name=<string> | none for all
  Res (all): { "success": true, "profiles": [ { "profile_id": number, "profile_name": string, "profile_json": object } ] }
  Res (single): { "success": true, "profile": { "profile_id": number, "profile_name": string, "profile_json": object } }

- createProfile.php (POST)
  Req: { "profile_name": string, "profile_json": object }
  Res: { "success": true, "message": string, "profile_id": number } | 500 with { "success": false, "error": string }

- deleteProfile.php (POST)
  Req: { "profile_id": number }
  Res: { "success": true, "message": string } | 404/500 with { "success": false, "error": string }

- tuyaBlindsApi.php (GET)
  Query: ?action=open|close|status
  Res status: { "success": true, "battery_percent": number|null, "blinds_state": string|null }
  Res command: { "success": boolean, "result": any|null, "error": string|null }

- getEncryptionKeys.php (GET)
  Res: { "data": [ { "device_api_key": string, "device_encryption_key": string } ] }

- updateDeviceStatus.php (POST)
  Req: { "device_api_key": string }
  Res: { "success": true } | 404/500 with { "success": false, "error": string }

## Frontend profile JSON (examples)

Profiles (`profiles.profile_json`) drive multiple subsystems. Example rows from seed data:

- Work
  {
    "AC": { "channel": "air_conditioning", "payload": { "requestedTemp": 24 } },
    "WLED": { "on": false, "preset_name": "Wyłączony" },
    "blinds": { "state": "open" },
    "lights": { "channel": "main_lights", "payload": { "state": true } }
  }

- Automate blinds variant
  {
    "AC": { "channel": "air_conditioning", "payload": { "requestedTemp": 26 } },
    "WLED": { "on": false, "preset_name": "Wyłączony" },
    "blinds": { "maxLux": 2000, "minLux": 500, "automate": true },
    "lights": { "channel": "main_lights", "payload": { "state": true } }
  }

## Notes

- Device ↔ Server messages now require `msgIV` and hex `payload` per message.
- For ESP32-origin updates, always include `device_api_key`; the server uses it to decrypt and to update `last_seen`.
- Frontend WS subscriptions stay plaintext and follow the broadcast shapes shown above.
