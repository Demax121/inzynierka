# Complete Project Documentation {#complete-project-documentation}

## Table of Contents {#table-of-contents}
1. Executive Summary
2. Architectural Overview
3. Data Model & ER Diagram
4. Runtime Data Flows
5. Communication Protocol (WebSocket & Encryption)
6. Backend (PHP Endpoints)
7. Realtime Server (Bun WebSocket Hub)
8. Frontend (Vue + Pinia) Architecture
9. Firmware (ESP32) Architecture
10. Custom Libraries Rationale (AESCrypto, MyWiFi)
11. Security Model & Cryptography Choice
12. Profiles System (Cross‑Subsystem Configuration)
13. Error Handling & Resilience Patterns
14. Performance Considerations
15. Developer Workflow & Tooling
16. Component Catalogue (Vue)
17. Future Enhancements Roadmap
18. Glossary
19. Maintenance & Operations Checklist
 20. Device Simulation Quickstart

---
## 1. Executive Summary {#sec-1-executive-summary}
This project implements a compact, offline‑capable smart home environment with authenticated encrypted device telemetry, real‑time UI updates, and configurable automation. A deliberately simple stack (PostgreSQL + thin PHP + Bun WebSocket + Vue 3 + ESP32 firmware) maximizes transparency and educational value while retaining production‑adjacent patterns (encryption, debouncing, modular stores, key rotation refresh, structured messages).

## 2. Architectural Overview {#sec-2-architectural-overview}
Layer | Responsibility | Key Tech
------|---------------|---------
Device Tier | Sense & actuate (temp, humidity, light level, door, HVAC, lights) | ESP32, C++ (Arduino), mbedTLS
Transport | Low‑latency full‑duplex messaging | WebSocket (Bun ws)
Application API | Persistence, configuration, profile mgmt | PHP endpoints (PDO → PostgreSQL)
Data Store | Durable state & historical metrics | PostgreSQL (JSONB for flexible profile schema)
Presentation | Real‑time dashboard & controls | Vue 3 + Pinia + Vite
Security | Confidentiality + integrity between devices & server | AES-128-GCM (per‑device static key)

Rationale: Separate plain frontend (no secret material) from encrypted machine channel ensures minimal attack surface if UI is compromised.

## 3. Data Model & ER Diagram {#sec-3-data-model-er}
```
+---------------+        +-----------------+
|   devices     |        |   room_statistics|
|---------------|        |-----------------|
| device_id PK  |   +--> | stat_id PK      |
| device_name   |   |    | stat_date       |
| device_mac UQ |   |    | stat_temperature|
| device_api_key|   |    | stat_humidity   |
| device_encryption_key UQ| | stat_pressure |
| device_last_seen       | |
+---------------+   |    +-----------------+
                    |
+---------------+   |    +-----------------+
|  door_status  |   |    |   profiles       |
|---------------|   |    |------------------|
| status_id PK  |   |    | profile_id PK    |
| status_date   |   |    | profile_name     |
| status_name   |   |    | profile_json JSONB|
+---------------+   |    +------------------+
                    |
               +------------------+
               |  blinds_config   |
               |------------------|
               | ID_config PK     |
               | min_lux          |
               | max_lux          |
               | automate         |
               +------------------+
```
Relationships are loose (no foreign keys) to allow device registration and telemetry ingestion decoupled from strict relational constraints. Profiles encapsulate multi‑device config snapshots using JSONB.

## 4. Runtime Data Flows {#sec-4-runtime-data-flows}
Flow | Steps
-----|------
Telemetry (room_stats) | ESP32 encrypts JSON → WS server decrypts → broadcasts plaintext to frontends → cached last temperature → pushes encrypted temperature to AC device
Command (main_lights) | Frontend sends plaintext `{channel, lightON}` → server encrypts per light controller connection → device decrypts & toggles → device reports status (encrypted) → server rebroadcasts plaintext
Door Event | Door sensor sends encrypted `{doorOpen}` → server broadcasts → frontend store debounces persistence → PHP `saveDoorStatus.php` persists
Profile Apply | Frontend selects profile → for each subsystem dispatch WS / HTTP actions (lights, AC, blinds, WLED) → devices act on updated commands (some re‑encrypted by server)

## 5. Communication Protocol {#sec-5-communication-protocol}
Device Envelope (encrypted):
```
{ identity, channel, device_api_key, nonce, payload, tag, alg? }
```
- `nonce`: 12-byte random (hex 24 chars)
- `payload`: hex ciphertext of JSON body
- `tag`: 16-byte authentication tag (hex 32 chars)
- `alg`: optional marker `AES-128-GCM`

Frontend Broadcast (plaintext): Channel-specific flattened JSON omitting encryption fields for UI immediacy.

## 6. Backend (PHP Endpoints) {#sec-6-backend-php-endpoints}
Patterns:
- Common headers (CORS + JSON) + method guard
- Uniform success envelope `{success:true,...}` / error envelope `{success:false,error}`
- Prepared statements (PDO) for all variable inputs
- Validation (type, range, semantic: min_lux < max_lux)

Key Endpoints:
- Blinds config (get/save + automate toggle)
- Door / room stats saving
- Profiles CRUD (create, list, delete, choose)
- Tuya blinds proxy (open/close/status)
- Device key enumeration / last seen update

## 7. Realtime Server (Bun) {#sec-7-realtime-server-bun}
Responsibilities:
- Maintain map: socket → { type, channel, device_api_key }
- Refresh encryption key map every 5 min
- Decrypt device frames → shape validation → broadcast plaintext
- Encrypt targeted commands (main_lights, air_conditioning, temperature push)
- Heartbeat (ping) every 10s to devices; devices respond by re‑sending state

Channel Logic Highlights:
- `room_stats`: caches last temperature; triggers encrypted push to AC devices
- `air_conditioning`: merges device state (encrypted) with `currentTemp` for frontend context
- `main_lights`: differentiates device status vs. frontend command paths

## 8. Frontend (Vue + Pinia) {#sec-8-frontend-vue-pinia}
Stores Abstract:
- `wsStore`: central WebSocket connection / URL
- `doorStatusStore`: debounced queue for door status persistence
- `saveStatsStore`: hourly persistence throttle
- `automateStore`: blinds automation state

Components: Each card subscribes to relevant channel messages and exposes minimal UI actions (toggle light, adjust AC temperature, apply profile, etc.). BEM‑style SCSS and design tokens unify visuals.

Reconnect Strategy: WebSocket onmessage/onclose handlers schedule reconnect with 5s backoff.

## 9. Firmware (ESP32) {#sec-9-firmware-esp32}
Sketch Responsibilities (per device):
- Connect WiFi (MyWiFi)
- Establish WS → identify with channel + api_key
- Periodic telemetry or event-driven updates
- Encrypt outbound frames (AES-128-GCM)
- Decrypt inbound commands (if applicable)

AC Controller Specific:
- Hysteresis control with ±2.0°C band
- Manual override button sets state and immediate mode evaluation
- Display rendering (ILI9341) with dynamic fields

Door Sensor / Lux / Room Stats:
- Simple measurement loops + conditional send intervals
- Ensure fresh nonce each frame

Main Lights Controller:
- Reflect state changes from frontend or manual hardware events (if extended)

## 10. Custom Libraries Rationale {#sec-10-custom-libraries-rationale}
Library | Reason | Notes
--------|--------|------
AESCrypto | Encapsulation of mbedTLS complexity & migration from CBC to GCM | Provides nonce generation, hex helpers, symmetric API
MyWiFi | Remove repetitive WiFi boilerplate & reconnection patterns | Could evolve to support captive portal / provisioning

## 11. Security Model & Encryption Choice {#sec-11-security-model-encryption}
Why AES-128-GCM:
- Authenticated encryption (solves integrity gap left by CBC)
- Hardware-accelerated on ESP32 (mbedTLS)
- Simplicity: single pass encrypt+tag; no padding

Threat Model Considerations:
- LAN attacker sniffing frames: sees only ciphertext (frontend remains plaintext but carries no secrets)
- Tampering: GCM tag rejects modified frames
- Replay: Not yet mitigated (nonce uniqueness + potential future nonce cache)

Rejected Alternatives:
- TLS directly on device (heavier handshake, cert management overhead)
- Diffie-Hellman key exchange (overkill for static lab environment)
- Custom binary framing (less debuggable)

## 12. Profiles System {#sec-12-profiles-system}
Profiles unify multi-subsystem configuration (AC, lights, blinds, WLED). JSONB enables ad-hoc expansion without schema migrations. Applying a profile triggers each channel's native transport (WS or HTTP) for atomic feel at UI layer.

## 13. Error Handling & Resilience {#sec-13-error-handling-resilience}
Pattern | Technique
--------|----------
Device reconnect | WebSocketsClient auto + manual watchdog
WiFi reconnect | MyWiFi loop() periodic retry
Door status burst | Debounce queue flush
Key rotation | Timed refresh of encryption map
Ping keepalive | 10s interval; device echoes state

## 14. Performance Considerations {#sec-14-performance-considerations}
- Low device count → O(n) broadcast acceptable
- Potential optimization: per-channel subscription sets
- DB write throttling (stats hourly; door events only on change)
- GCM overhead negligible vs. network latency for small JSON bodies (< 200B)

## 15. Developer Workflow & Tooling {#sec-15-developer-workflow-tooling}
- Docker compose spin-up for DB, backend, frontend, WS
- Local hot reload (Vite + Bun)
- Simulator script (`testDeviceSim.js`) for encrypted channel debugging

### Device Simulation Quickstart {#sec-15-device-simulation-quickstart}
Run a sample encrypted frame without real hardware:
```
cd jsServer
bun run testDeviceSim.js room_stats <device_api_key> <16charKey>
```
Supported channels: `room_stats`, `door_sensor`, `lux_sensor`, `main_lights`, `air_conditioning`.
What it does:
1. Sends identification
2. Generates 12B nonce, encrypts canonical JSON with AES-128-GCM
3. Sends encrypted frame; prints server responses
Integrity test: modify one hex nibble in `payload` -> server decrypt fails silently.

## 16. Component Catalogue (Vue) {#sec-16-component-catalogue-vue}
Component | Purpose | Key Interactions
----------|---------|-----------------
`doorsCard_component` | Display & persist door state | Debounced POST
`roomStatsCard_component` | Show temperature/humidity/pressure | Hourly persistence trigger
`luxSensorCard_component` | Show ambient lux | Drives blinds logic (future)
`mainLightsCard_component` | Toggle lights | WS command + reflect status
`airConditioningCard_component` | Adjust climate target / power | Sends requestedTemp / ON state
`blindsAutomateCard_component` | Configure automation thresholds | Validates min<max before POST
`blindsCard_component` | Show blinds state + Tuya actions | HTTP GET/command
`chooseProfileCard_component` | Apply profile across systems | Dispatch multi-channel actions
`createProfileCard_component` | Persist new profile | JSON assembly + POST
`wledCard_component` | Manage WLED (on/preset) | Direct HTTP commands
`Navbar_component` | Navigation / socket status (if extended) | Visual state only

## 17. Future Enhancements Roadmap {#sec-17-future-enhancements-roadmap}
Category | Item
--------|-----
Security | AAD binding (channel + api_key) in GCM
Security | Replay detection (nonce LRU cache)
Observability | Structured JSON logs + log rotation
Performance | Channel subscription map
Resilience | Heartbeat timeouts / stale socket pruning
UX | Toast notifications system
Data | Indices on frequently filtered columns

## 18. Glossary {#sec-18-glossary}
Term | Meaning
-----|--------
Envelope | Outer encrypted JSON wrapper (nonce, payload, tag)
Profile | Multi-subsystem configuration snapshot
Hysteresis | Control band preventing rapid HVAC toggling
Nonce | Single-use random value ensuring AEAD uniqueness

## 19. Maintenance & Operations Checklist {#sec-19-maintenance-operations-checklist}
Task | Frequency
-----|----------
Rotate device keys | Quarterly
Inspect decrypt failures | Weekly
Vacuum / prune historical tables | Monthly
Review profile proliferation | Quarterly
Security review (dependencies) | Quarterly

---
Generated programmatically; keep updated alongside code changes.

## 20. Device Simulation Quickstart {#sec-20-device-simulation-quickstart}
Reference duplication for quick navigation (see section 15):
```
cd jsServer
bun run testDeviceSim.js main_lights <device_api_key> <16charKey>
```
Change first arg to the desired channel.
