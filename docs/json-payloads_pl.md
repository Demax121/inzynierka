# Referencja JSON Payload (PL) {#pl-referencja-json-payload}

## Szyfrowanie {#pl-szyfrowanie}
- Algorytm: AES-128-GCM
- Pola koperty: `channel`, `device_api_key`, `nonce`, `payload`, `tag`, (`alg` opcjonalnie)
- `nonce`: 24-hex (12 bajtów), `tag`: 32-hex (16 bajtów), `payload`: hex szyfrogram JSON

## Identyfikacja {#pl-identyfikacja}
```json
{ "type": "esp32_identification", "channel": "room_stats", "device_api_key": "<apiKey>" }
```

## Kanały {#pl-kanaly}
### door_sensor {#pl-channel-door-sensor}
Urządzenie → Serwer (szyfrowane): `{ doorOpen: bool }`
Serwer → Frontend: `{ channel:"door_sensor", payload:{ doorOpen: bool } }`

### room_stats {#pl-channel-room-stats}
Urządzenie → Serwer: `{ temperature, humidity, pressure }`
Serwer → Frontend: `{ channel:"room_stats", temperature, humidity, pressure }`
Serwer → AC (push temp): `{ channel:"air_conditioning", temperature }` (szyfrowane do ESP32)

### lux_sensor {#pl-channel-lux-sensor}
Urządzenie → Serwer: `{ lux }`
Frontend: `{ channel:"lux_sensor", lux }`

### main_lights {#pl-channel-main-lights}
Frontend → Serwer: `{ channel:"main_lights", lightON }`
Urządzenie → Serwer: `{ lightON }`
Serwer → Frontend: `{ channel:"main_lights", lightON }`
Serwer → Urządzenie: `{ lightON }` (zaszyfrowane)

### air_conditioning {#pl-channel-air-conditioning}
Urządzenie → Serwer: `{ requestedTemp, function, klimaON, manualOverride }`
Frontend → Serwer: `{ channel:"air_conditioning", payload:{ requestedTemp?, klimaON?, manualOverride? } }`
Serwer → Frontend: `{ channel:"air_conditioning", payload:{ requestedTemp, function, klimaON, manualOverride, currentTemp } }`
Serwer → Urządzenie (temp push): `{ temperature }`

## Endpointy PHP (skrót) {#pl-endpointy-php}
Nazwa | Metoda | Wejście | Wyjście
------|--------|--------|--------
`saveRoomStats.php` | POST | `{temperature,humidity,pressure}` | `{success:true}`
`getDoorStatus.php` | GET | - | `{success:true,data:[...]}`
`saveDoorStatus.php` | POST | `{status}` | `{success:true}`
`getBlindsConfig.php` | GET | - | `{min_lux,max_lux,automate}`
`saveBlindsConfig.php` | POST | `{min_lux,max_lux,automate}` | `{success:true,...}`
`setBlindsAutomate.php` | POST | `{automate}` | `{success:true,automate}`
`chooseProfile.php` | GET | `?id|name` | `{profile|profiles}`
`createProfile.php` | POST | `{profile_name,profile_json}` | `{success:true,profile_id}`
`deleteProfile.php` | POST | `{profile_id}` | `{success:true}`
`tuyaBlindsApi.php` | GET | `?action=` | `{success:..., ...}`
`getEncryptionKeys.php` | GET | - | `{data:[{device_api_key,device_encryption_key}]}`
`updateDeviceStatus.php` | POST | `{device_api_key}` | `{success:true}`

## Profile (przykład) {#pl-profile-przyklad}
```json
{
  "AC": { "channel": "air_conditioning", "payload": { "requestedTemp": 24 } },
  "WLED": { "on": false, "preset_name": "Wyłączony" },
  "blinds": { "state": "open" },
  "lights": { "channel": "main_lights", "payload": { "state": true } }
}
```

## Uwagi {#pl-uwagi}
- Zawsze dołącz `device_api_key` dla ramek z urządzenia.
- Frontend nie widzi kluczy – serwer pośredniczy w szyfrowaniu.

### Symulacja Urządzenia (Quickstart) {#pl-symulacja-quickstart}
```
cd jsServer
bun run testDeviceSim.js lux_sensor <device_api_key> <16znakowyKlucz>
```
Kanały: door_sensor, room_stats, lux_sensor, main_lights, air_conditioning.
Test integralności: zmień znak w `payload` – ramka zostanie odrzucona.
