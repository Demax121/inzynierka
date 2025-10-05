# Engineering Project Documentation {#root-engineering-project-documentation}

## 1. Overview {#root-sec-1-overview}
This project is an offline-capable smart home control & telemetry stack composed of:
- PostgreSQL database (state, telemetry, configuration)
- PHP (Caddy + PHP-FPM) thin JSON endpoints
- Bun WebSocket server for real-time encrypted device ↔ server messaging
- Vite + Vue 3 + Pinia frontend (plaintext WS consumption and UI controls)
- ESP32 firmware devices (room stats sensor, lux sensor, door sensor, main lights controller, AC controller) using AES-128-GCM encryption

### High-Level Flow {#root-high-level-flow}
1. ESP32 device connects to WS server, sends identification `{type:"esp32_identification", channel, device_api_key}`.
2. Device sends encrypted frames `{channel, device_api_key, nonce, payload, tag}` (payload = hex ciphertext of JSON body via AES-128-GCM; tag = 16-byte auth tag).
3. Server decrypts, validates, updates `device_last_seen`, and broadcasts simplified plaintext to all frontends.
4. Frontend dispatches user commands (lights, AC) in plaintext WS messages; server encrypts per-device when forwarding to ESP32.
5. Periodic or event-driven values are persisted through PHP REST-like endpoints into PostgreSQL.

## 2. Rationale / Design Decisions {#root-sec-2-rationale-design}
| Aspect | Choice | Rationale | Alternatives Rejected |
|--------|--------|-----------|------------------------|
| API Layer | Thin PHP endpoints | Fast iteration, low overhead, simple PDO usage | Full Node REST API (would duplicate WS server responsibilities) |
| Real-time | Dedicated Bun WS server | Clear separation of stateless WS logic and stateful DB writes | Embedding WS inside PHP stack (complex) |
| Encryption | AES-128-GCM device ↔ server only | Confidentiality + integrity; keeps secrets server-side | Frontend encryption (would leak keys); TLS-only (still leaves LAN plaintext) |
| Data Storage | PostgreSQL | Strong relational and JSONB support | SQLite (concurrency), MySQL (no advantage here) |
| Frontend | Vue 3 + Pinia | Reactive store pattern, lightweight, Vite dev speed | React (heavier for small scope) |
| Device Messages | Per-channel JSON inside encrypted envelope | Extensible, easily debug via simulator | Binary custom protocol (harder maintainability) |

## 3. Repository Structure {#root-sec-3-repository-structure}
```
backend/               # PHP + Caddy configs
  php/                 # Endpoints (stateless, validated)
  Caddyfile            # Static & fastcgi + CORS headers
jsServer/              # Bun WebSocket server (encryption, routing)
frontend/              # Vue 3 application (Pinia stores, components)
IOT/ino/               # ESP32 sketches (one per device type)
postgreSQL/            # DB Docker build + init.sql schema + seed
docs/                  # Documentation (this file, payload references, etc.)
```

## 4. Database Schema Summary {#root-sec-4-database-schema}
Table | Purpose | Key Columns
------|---------|------------
`devices` | Device identity, encryption keys, heartbeat | `device_api_key`, `device_encryption_key`, `device_last_seen`
`room_statistics` | Periodic environmental readings | `stat_date`, `stat_temperature`, `stat_humidity`, `stat_pressure`
`door_status` | Event log (open/close transitions) | `status_date`, `status_name`
`profiles` | Saved aggregate configuration profiles | `profile_name`, `profile_json`
`blinds_config` | Single-row blinds automation thresholds | `min_lux`, `max_lux`, `automate`

### Schema Initialization {#root-schema-initialization}
Defined in `postgreSQL/init.sql`; container runs it on first launch. Extend by adding migrations or altering the init script (for dev resets).

## 5. WebSocket Protocol {#root-sec-5-websocket-protocol}
### Identification (Device → Server) {#root-identification-device-server}
```
{ "type": "esp32_identification", "channel": "room_stats", "device_api_key": "<apiKey>" }
```
### Encrypted Device Frame {#root-encrypted-device-frame}
```
{ "channel": "room_stats", "device_api_key": "<apiKey>", "nonce": "<24-hex>", "tag": "<32-hex>", "payload": "<cipher-hex>" }
```
Decrypted JSON body shapes per channel (examples):
- `door_sensor`: `{ doorOpen: true }`
- `room_stats`: `{ temperature: 23.4, humidity: 55.1, pressure: 1012 }`
- `main_lights`: `{ lightON: true }`
- `lux_sensor`: `{ lux: 347 }`
- `air_conditioning`: `{ requestedTemp, function, klimaON, manualOverride }`

### Frontend Broadcasts (Plaintext) {#root-frontend-broadcasts}
- Door: `{ channel:"door_sensor", payload:{ doorOpen:bool } }`
- Stats: `{ channel:"room_stats", temperature, humidity, pressure }`
- Lights: `{ channel:"main_lights", lightON }`
- Lux: `{ channel:"lux_sensor", lux }`
- AC: `{ channel:"air_conditioning", payload:{ requestedTemp,function,klimaON,manualOverride,currentTemp } }`

### Commands (Frontend → Server) {#root-commands-frontend-server}
```
{ "channel": "main_lights", "lightON": true }
{ "channel": "air_conditioning", "payload": { ... } }
```
Server encrypts commands per recipient device.

## 6. Encryption Details {#root-sec-6-encryption-details}
Algorithm: AES-128-GCM (16 byte key from `device_encryption_key` column)

Per message:
- 12-byte random nonce (hex `nonce` – 24 hex chars)
- 16-byte auth tag (hex `tag` – 32 hex chars)
- Hex ciphertext of JSON body (`payload`)
- Optional `alg: "AES-128-GCM"` for clarity

Operational Notes:
- Keys cached and refreshed every 5 minutes (or on server restart)
- GCM provides authenticity; modified ciphertext/tag causes decrypt failure
- Nonce uniqueness per device enforced via hardware RNG; no padding required
- Frontend remains plaintext to avoid exposing keys

Example frame:
```json
{
  "channel":"door_sensor",
  "device_api_key":"<key>",
  "nonce":"a1b2c3d4e5f60718293a4b5c",
  "payload":"7fa9...",
  "tag":"d3c2b1a09f8e7d6c5b4a392817161514",
  "alg":"AES-128-GCM"
}
```

## 7. PHP Endpoint Conventions {#root-sec-7-php-endpoints}
All endpoints now:
- Include `bootstrap.php` for headers & helpers.
- Enforce method with `require_get()` / `require_post()`.
- Parse JSON via `read_json()`.
- Return `{ success: true, ... }` on success or `{ success:false, error:"message" }` with appropriate HTTP status.

Helper functions in `bootstrap.php`:
- `send_json($data, $code=200)`
- `fail($message, $code=400)`
- `get_pdo()` (singleton PDO)
- Validation wrappers and method guards.

## 8. Frontend Architecture {#root-sec-8-frontend-architecture}
### Stores (Pinia) {#root-stores-pinia}
Store | Purpose
------|--------
`wsStore` | Provides WebSocket URL from env.
`linkStore` | Builds backend PHP endpoint URLs.
`doorStatusStore` | Tracks last door state (debounced persistence).
`saveStatsStore` | Hourly (or forced) room stats persistence.
`automateStore` | Blinds automation state UI binding.

Utility modules:
- `utils/api.js`: unified `getJson`, `postJson` returning normalized result objects.
- `utils/wsHelpers.js`: safe message parsing + type guards.

### Debounce Mechanics (Door Status) {#root-debounce-door-status}
- `enqueueStatus()` collects rapid toggles; flush after adjustable delay (default 5000 ms) or immediate if delay set to 0.

### Room Stats Persistence {#root-room-stats-persistence}
- Stores the most recent reading; after one hour (or forced save) sends to backend if last save timestamp expired.

## 9. ESP32 Device Responsibilities {#root-sec-9-esp32-responsibilities}
Each sketch:
1. Connect Wi-Fi, open WS.
2. Identify (`esp32_identification`).
3. Assemble JSON → encrypt with key + random 12-byte nonce (GCM) → envelope with nonce + tag.
4. Send periodic telemetry or event updates.
5. Decrypt & authenticate inbound commands (lights / AC) and act.

Libraries & Crypto:
- Custom `AESCrypto` wrapper around mbedtls performing AES-128-GCM (no padding).
- Nonce generation: 12 bytes from `esp_random()` seeded by Wi-Fi hardware RNG.

## 10. Development Setup {#root-sec-10-development-setup}
### Prerequisites {#root-prerequisites}
- Docker + Docker Compose
- Bun (for local dev of WS or frontend if outside container)
- Node (optional, not required if using Bun exclusively)

### Environment Variables {#root-environment-variables}
`.env` (example keys):
```
FRONTEND_PORT_EX=8882
BACKEND_CADDY_PORT_EX=8883
BUN_API_PORT_EX=3000
POSTGRES_PORT_EX=5433
VITE_WS_URL_PREFIX=ws://localhost:3000
VITE_BACKEND_URL_PREFIX=http://localhost:8883/
VITE_WLED_URL_PREFIX=http://<wled-ip>
```

### Run (Docker) {#root-run-docker}
```
docker compose up -d
```
Access:
- Frontend: http://localhost:8882
- PHP Endpoints: http://localhost:8883/<file>.php
- WebSocket: ws://localhost:3000
- pgAdmin (if configured): expose relevant port

### Local Frontend (outside Docker) {#root-local-frontend}
```
cd frontend
bun install
bun run dev
```

### Local WS Server (outside Docker) {#root-local-ws-server}
```
cd jsServer
bun install
bun bunServer.js
```

### Device Simulation (Encrypted Frame Quickstart) {#root-device-simulation-quickstart}
Use the built-in simulator to emulate an ESP32 sending an encrypted frame:
```
cd jsServer
bun run testDeviceSim.js room_stats <device_api_key> <16charEncryptionKey>
```
Replace `room_stats` with one of: `door_sensor`, `lux_sensor`, `main_lights`, `air_conditioning`.
What happens:
- Script generates a 12-byte nonce, encrypts a channel-appropriate JSON using AES-128-GCM
- Sends identification + encrypted payload
- Logs any decrypted server responses (e.g. AC temperature pushes)
If you tamper with `payload` or `tag` before it reaches the server, decryption will fail silently (integrity check).

## 11. Testing & Debugging {#root-sec-11-testing-debugging}
| Scenario | Approach |
|----------|----------|
| Verify encryption | Use `jsServer/testDeviceSim.js` to emit encrypted frames |
| CORS issues | Confirm Caddy headers + ensure correct endpoint filename (not just base URL) |
| Device not updating | Check WS logs for identification; ensure key present in DB |
| Blinds config invalid | Endpoint returns 400 with validation message |
| Room stats not saved | Check localStorage timestamp & network tab; forced save via store `forceSaveNow()` |

### Simulator Usage {#root-simulator-usage}
```
bun run jsServer/testDeviceSim.js room_stats <apiKey> <16charKey>
```

### Common Pitfalls {#root-common-pitfalls}
- Forgetting to append `.php` filename → root POST fails CORS/404.
- Using stale encryption key after DB update (wait for 5 min refresh or restart WS server).
- Large drift in device clock irrelevant (server timestamps all DB writes).

## 12. Extending the System {#root-sec-12-extending}
### Add a New WebSocket Channel {#root-add-ws-channel}
1. Define device JSON body schema.
2. Update device firmware to send encrypted envelope.
3. Add handler branch in `bunServer.js` (decrypt, validate, broadcast).
4. Create Vue component + store if needed.
5. Optionally add persistence endpoint & DB table.

### Add a New PHP Endpoint {#root-add-php-endpoint}
1. Copy minimal template:
```php
<?php
require_once __DIR__.'/bootstrap.php';
require_post();
$data = read_json();
// validate ...
$pdo = get_pdo();
// perform action
send_json(['success'=>true]);
?>
```
2. Expose via front-end using `getPhpApiUrl()`.

### Add a Profile Field {#root-add-profile-field}
- Update profile JSON creation in frontend.
- No schema migration needed (JSONB flexible), but document new key in payload reference.

## 13. Security Considerations {#root-sec-13-security}
- Encryption keys never exposed to browser.
- All device frames validated after decrypt for expected shape.
- Potential future: rate limiting device messages, signature/HMAC to detect tampering (CBC alone doesn’t authenticate), rotate keys via DB update + short overlap.

## 14. Performance Notes {#root-sec-14-performance}
- WS broadcast loops scale linearly with connections; acceptable for low device count (lab / home). For scale-out: consider tracking per-channel subscriber sets.
- DB writes batched logically: door status only on change (debounced on frontend), stats hourly.

## 15. Future Improvements {#root-sec-15-future-improvements}
Category | Idea
---------|-----
Security | Add HMAC (Encrypt-then-MAC) for authenticity
Observability | Structured JSON logging & log rotation
Frontend | Toast notification system for failed saves
Backend | Pagination for `getDoorStatus.php` (historic trimming)
WS | Heartbeat timeout detection & auto-clean stale connections
Database | Add indices (e.g., `CREATE INDEX ON room_statistics(stat_date DESC)`)
Docs | Diagram images for architecture + sequence flow

## 16. Glossary {#root-sec-16-glossary}
Term | Definition
-----|-----------
Envelope | Outer JSON wrapper containing `channel`, `nonce`, `tag`, `payload` (ciphertext)
Frontend broadcast | Plaintext WS message from server to browsers
ESP32 identification | Initial unencrypted JSON to bind connection to device key
Key refresh | Periodic reload of encryption keys from DB every 5 minutes

## 17. Troubleshooting Quick Table {#root-sec-17-troubleshooting}
Symptom | Likely Cause | Fix
--------|--------------|----
CORS error on POST | Missing `.php` in fetch URL | Use `linkStore.getPhpApiUrl('file.php')`
Decryption failure log | Wrong key / truncated payload | Verify DB key & device firmware constant
No room temperature in AC payload | `lastRoomTemperature` not yet set | Wait until first room_stats frame arrives
Door status duplicates | Not using debounce | Use `enqueueStatus()`
Blinds config save 400 | `min_lux >= max_lux` | Adjust values

## 18. Maintenance Checklist {#root-sec-18-maintenance}
- Rotate device keys quarterly (update DB + reflash devices)
- Vacuum / analyze DB growth (door & stats tables) monthly
- Review logs for decrypt warnings
- Rebuild containers after dependency updates

## 19. Conclusion {#root-sec-19-conclusion}
This architecture balances simplicity (thin PHP, plaintext frontend) with secure device communication (encrypted WS payloads). Modularity allows incremental enhancement (auth, metrics, HMAC) without redesigning core flows.

---
Generated as part of engineering thesis documentation. Extend and version control this file with any architectural changes.
