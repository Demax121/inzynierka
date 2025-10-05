# Offline smart‑home frontend (Vue 3 + Vite) {#frontend-readme-root}

This frontend is part of the offline stack (Vue 3 + Vite + Pinia). For repo-wide guidance tailored to AI coding agents and contributors, see:

- `.github/copilot-instructions.md` — architecture, workflows, conventions
- `docs/json-payloads.md` — full JSON layouts for WS channels, profiles, and PHP endpoints

The app uses Vite with Bun for dev scripts. Global SCSS via `@use "@/scss" as *;`, and `@` alias resolves to `src`.

## Encryption overview (devices ↔ server) {#frontend-encryption-overview}

- Algorithm: AES-128-GCM (AEAD, no padding).
- Key: per-device `device_encryption_key` (first 16 chars) stored only on server + device (never exposed to browser).
- Nonce: 12 random bytes per message (24 hex chars) as `nonce`; 16-byte auth tag (32 hex) as `tag`.
- Ciphertext: hex in `payload` field (encrypted JSON body per channel).
- Frontend broadcasts remain plaintext; only device ↔ server traffic is encrypted.

Device envelope example:
{
	"identity": "<device_name>",
	"channel": "<channel>",
	"device_api_key": "<apiKey>",
	"nonce": "<24-hex>",
	"tag": "<32-hex>",
	"payload": "<hex-cipher>" // decrypts to channel JSON
}

Server → device command (same structure; identity may be omitted but channel + nonce + tag + payload required):
{ "channel": "main_lights", "nonce": "<24-hex>", "tag": "<32-hex>", "payload": "<hex-cipher>" }

## Testing encrypted flow {#frontend-testing-encrypted-flow}

1. Start Bun WS server (port 3000 inside container or expose via compose) and ensure devices table has matching api/encryption keys.
2. Run simulator (adjust ws URL via SIM_WS env if not localhost:3000):
	 ```bash
	 bun run jsServer/testDeviceSim.js main_lights <apiKey> <encryptionKey16>
	 ```
3. Observe server logs decrypting payload and broadcasting plaintext to frontends.

Channels supported: door_sensor, room_stats, lux_sensor, main_lights, air_conditioning.

For more detail see `docs/json-payloads.md` and `.github/copilot-instructions.md` (Encryption guide section).
