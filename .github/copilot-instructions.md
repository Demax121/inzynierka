# AI agent guide for this repo

This stack runs offline in Docker: PostgreSQL + pgAdmin, PHP (via Caddy) for JSON endpoints, a Bun WebSocket server for realtime, and a Vite + Vue 3 frontend.

## Key structure
- docker-compose.yml — service names are used as internal DNS (e.g., offline_backend_server_caddy_dyplom)
- backend/
  - Caddyfile — serves /var/www/php with CORS
  - php/*.php — thin JSON endpoints (GET for reads, POST for writes)
  - php/config.php — DB+Tuya creds (hard-coded; used by all endpoints)
- jsServer/bunServer.js — WS hub for devices/frontends; also calls PHP via HTTP
- frontend/
  - src/stores/*.js — wsStore (VITE_WS_URL_PREFIX), linkStore (VITE_BACKEND_URL_PREFIX), others
  - src/components/* — cards implement WS patterns (doors, lights, room stats, blinds)
- postgreSQL/init.sql — tables: devices, room_statistics, door_status, profiles, blinds_config (with seed)

## How services talk
- Frontend ↔ Bun WS: WebSocket at VITE_WS_URL_PREFIX
- ESP32 ↔ Bun WS: identify with {type:"esp32_identification", channel}
- Bun WS → PHP: HTTP to http://offline_backend_server_caddy_dyplom/<file>.php
- PHP → PostgreSQL: PDO using config.php constants

## WebSocket channels (shapes from bunServer.js)
- door_sensor: device → {channel, device_api_key, payload:{doorOpen:bool}}; frontend receives same shape
- room_stats: device → {channel, device_api_key, payload:{temperature,humidity,pressure}}; frontend receives {channel,temperature,humidity,pressure}
- main_lights: frontend command → {channel, lightON:bool}; device status → {channel, device_api_key, payload:{lightON}}; broadcasts mirror lightON
- lux_sensor: device → {channel, device_api_key, payload:{lux:number}}; frontend receives {channel,lux}
- air_conditioning: device → {channel, device_api_key, payload:{requestedTemp,function,klimaON,manualOverride}}; frontend receives payload plus currentTemp; frontend command forwards payload to esp32 on that channel

## PHP endpoint patterns
- All set JSON headers, handle OPTIONS, return JSON. Reads via GET, writes via POST with JSON body.
- Examples:
  - saveRoomStats.php: POST {temperature,humidity,pressure}
  - saveDoorStatus.php: POST {status:"Door open"|"Door closed"}; getDoorStatus.php (GET history)
  - getBlindsConfig.php (GET); saveBlindsConfig.php POST {min_lux,max_lux,automate}; setBlindsAutomate.php POST {automate}
  - chooseProfile.php GET[?id|name]; createProfile.php POST {profile_name,profile_json}; deleteProfile.php POST {profile_id}
  - tuyaBlindsApi.php GET ?action=open|close|status
  - getEncryptionKeys.php (GET), updateDeviceStatus.php POST {device_api_key}

## Data you’ll touch often
- devices: (device_api_key, device_encryption_key, device_last_seen)
- room_statistics: (stat_date, stat_temperature, stat_humidity, stat_pressure)
- door_status: (status_date, status_name)
- profiles: (profile_json jsonb)
- blinds_config: (min_lux, max_lux, automate)

## Frontend config
- wsStore.wsUrl = import.meta.env.VITE_WS_URL_PREFIX
- linkStore.getPhpApiUrl(name) = VITE_BACKEND_URL_PREFIX + name
- Some stores post directly to VITE_BACKEND_URL_PREFIX expecting it to be a full endpoint URL (check doorStatusStore, saveStatsStore)

## Run/dev workflows
- Docker: provide .env for ports (FRONTEND_PORT_EX/NATIVE, BACKEND_CADDY_*, BUN_API_*), then `docker compose up -d` (service names are used inside the network)
- Frontend (host): in frontend/, `bun run dev` (Vite). Alias @ → src; global SCSS via `@use "@/scss" as *;`
- Bun WS (host): in jsServer/, `bun bunServer.js`; ensure dependencies for ws if running outside container

## Conventions & gotchas
- ESP32-origin WS messages must include device_api_key and nested payload; call updateLastSeen for those updates
- Bun server refreshes encryption keys every 5 min (currently not used for crypto, but keep API keys stable)
- Adding a WS channel: extend bunServer.js handlers; mirror shapes in components/stores; broadcast rules follow existing patterns
- New PHP endpoints: copy header/CORS/JSON style, use PDO prepared statements, and expose via Caddy path
- WLED is controlled directly from frontend (linkStore.wledIP), not via backend

If any env names/URLs are unclear (e.g., VITE_* defaults), ask to align code and docs or provide a sample .env.

See also: docs/json-payloads.md for complete JSON layouts (WS messages, profile shapes, and PHP request/response bodies).

## Prompting guide

- Choose the right mode
  - Agent mode: multi-step, cross-service work (WS + PHP + Vue + SQL). Let the agent read files and coordinate edits end-to-end.
  - Edits mode: narrow, well-scoped changes in known files with clear acceptance criteria.
  - Inline file chat: small refactors or function-level tweaks within a single file.
  - Chat Q&A: explore behavior or confirm message/JSON shapes before editing.

- Write effective prompts (for this repo)
  - Outcome first: describe the user-visible/API result you expect.
  - Point to exact paths: e.g., `jsServer/bunServer.js`, `backend/php/saveBlindsConfig.php`, specific Vue components/stores.
  - Include WS/PHP JSON shapes explicitly (channel names, payload keys, status codes).
  - Acceptance criteria: UI state, WS broadcasts, DB rows, and error handling behavior.
  - Constraints: follow CORS/headers + PDO prepared statements; use `linkStore.getPhpApiUrl(name)`; use service DNS from docker-compose (e.g., `offline_backend_server_caddy_dyplom`).

- Repo-specific prompt templates
  1) Add a WebSocket channel end-to-end
     - "Add channel 'window_tilt'. In `jsServer/bunServer.js` accept device → {channel:'window_tilt', device_api_key, payload:{tilt:number}}; broadcast to frontends: {channel:'window_tilt', tilt} and call updateLastSeen. Create `frontend/src/components/windowTiltCard_component.vue` that subscribes and shows tilt. Acceptance: sending tilt=37 updates the card to 37° without reload."
  2) Create a PHP endpoint + wire UI
     - "Add `backend/php/getDeviceLastSeen.php` (GET) returning [{device_api_key, device_last_seen}] using the same header/CORS/PDO style as `getEncryptionKeys.php`. Add an admin card listing results via `linkStore.getPhpApiUrl('getDeviceLastSeen.php')`. Acceptance: rows render; server 500 shows an error message in UI."
  3) Strengthen validation and surface it
     - "In `saveBlindsConfig.php`, return 400 when min_lux ≥ max_lux with {success:false,error:'...'}; update `blindsAutomateCard_component.vue` to block saving and show ‘Min lux must be less than Max lux’."
  4) Apply a profile across subsystems
     - "When applying in `chooseProfileCard_component.vue`: for lights send WS {channel:'main_lights', lightON:boolean}; for AC send WS {channel:'air_conditioning', payload:{requestedTemp,...}}; for blinds if state is a string then POST `setBlindsAutomate.php` false and GET `tuyaBlindsApi.php?action=open|close`, else POST `saveBlindsConfig.php` {min_lux,max_lux,automate}. Acceptance: UI reflects applied settings; WS broadcasts are observed."

- Quick do/don’t
  - Do: "Update `bunServer.js` to avoid echoing `main_lights` status back to the sender; acceptance: no double toggles in the UI."
  - Don’t: "Make it better." (too vague)

## Encryption guide (AES-128-CBC)

Scope
- Device ↔ Server traffic is encrypted per device; frontend broadcasts remain plaintext.

Envelope (device → server)
{ identity, channel, device_api_key, nonce, payload, tag, alg }
- nonce: 24‑hex (12-byte AES-GCM nonce)
- payload: hex ciphertext of JSON body (channel-specific)
- tag: 32-hex (16-byte authentication tag)
- alg: 'AES-128-GCM'
- Key: devices.device_encryption_key (exactly 16 chars used directly)

Server flow
1. On startup: fetch all {device_api_key, device_encryption_key} into Map.
2. On `esp32_identification`: store {channel, device_api_key} on ws connection.
3. When a device message arrives: decrypt using (key, nonce, tag) → apply logic → broadcast plaintext to frontends.
4. When sending to a device: build JSON body, encrypt with device key + fresh nonce, send {channel, nonce, payload, tag, alg}.

Device flow
1. Build JSON body (e.g., {doorOpen:true}).
2. Generate IV (esp_random) → hex.
3. AES-128-CBC encrypt with key.
4. Send envelope.
5. On inbound server command: decrypt using nonce + tag + key → act.

Testing (simulator)
Use `jsServer/testDeviceSim.js`:
  bun run jsServer/testDeviceSim.js main_lights <apiKey> <encryptionKey16>
Adjust channel to test others (room_stats, lux_sensor, air_conditioning, door_sensor).

Adding a new encrypted channel
1. Update device sketch to build clear JSON payload → envelope.
2. Add decrypt+handler branch in `bunServer.js` mirroring existing patterns.
3. Broadcast plaintext to frontend (unless you intentionally add frontend encryption wrapper).
4. If server also sends commands to device, implement encryption with `encryptPayloadForDevice`.

Frontend encryption (optional future)
- Currently frontend→server commands (e.g., main_lights toggle) are plaintext; server re-encrypts per device.
- To encrypt: wrap send in a utility that fetches device key (would require exposing keys to frontend → not recommended). Preferred: keep keys server-side only.

Reference: full layouts in `docs/json-payloads.md`.

