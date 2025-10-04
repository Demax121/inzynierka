# Offline smart‑home frontend (Vue 3 + Vite)

This frontend is part of the offline stack (Vue 3 + Vite + Pinia). For repo-wide guidance tailored to AI coding agents and contributors, see:

- `.github/copilot-instructions.md` — architecture, workflows, conventions
- `docs/json-payloads.md` — full JSON layouts for WS channels, profiles, and PHP endpoints

The app uses Vite with Bun for dev scripts. Global SCSS via `@use "@/scss" as *;`, and `@` alias resolves to `src`.

## Encryption overview (devices ↔ server)

- Algorithm: AES-128-CBC, PKCS7 padding.
- Key: per-device `device_encryption_key` (16 chars) stored only on server + device.
- Nonce: 12 random bytes per message (hex as `nonce`) and 16-byte auth tag (`tag`).
- Ciphertext: hex in `payload` field.
- Frontend broadcasts remain plaintext; only device traffic is encrypted.

Envelope sent by a device:
{
	"identity": "<device_name>",
	"channel": "<channel>",
	"device_api_key": "<apiKey>",
	"nonce": "<24-hex>",
	"tag": "<32-hex>",
	"payload": "<hex-cipher>" // decrypts to channel JSON
}

Server → device command (same structure, minus identity/device_api_key not strictly required but channel + nonce + tag + payload):
{ "channel": "main_lights", "nonce": "<24-hex>", "tag": "<32-hex>", "payload": "<hex-cipher>" }

## Testing encrypted flow

1. Start Bun WS server (port 3000 inside container or expose via compose) and ensure devices table has matching api/encryption keys.
2. Run simulator (adjust ws URL via SIM_WS env if not localhost:3000):
	 ```bash
	 bun run jsServer/testDeviceSim.js main_lights <apiKey> <encryptionKey16>
	 ```
3. Observe server logs decrypting payload and broadcasting plaintext to frontends.

Channels supported: door_sensor, room_stats, lux_sensor, main_lights, air_conditioning.

For more detail see `docs/json-payloads.md` and `.github/copilot-instructions.md` (Encryption guide section).
