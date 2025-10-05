# Dokumentacja Projektu Inżynierskiego (Wersja Skrócona – PL) {#pl-readme-skrocona}

## 1. Przegląd {#pl-readme-sec-1-przeglad}
Projekt to system sterowania i telemetryczny z obsługą HTTPS dla inteligentnego pomieszczenia:
- PostgreSQL – trwałość danych, statystyki, konfiguracje
- PHP (Caddy + PHP-FPM) – cienkie endpointy JSON
- Serwer WebSocket (Bun) – szyfrowana komunikacja urządzeń (ESP32 ↔ serwer)
- Frontend Vite + Vue 3 + Pinia – odbiór broadcastów (plaintext) i wysyłka komend
- Urządzenia ESP32 – sensory i sterowniki (statystyki, lux, drzwi, światło, klimatyzacja) używają AES-128-GCM

### Przepływ wysokopoziomowy {#pl-readme-przeplyw-wysokopoziomowy}
1. ESP32 łączy się z WS i wysyła identyfikację `{type:"esp32_identification", channel, device_api_key}`.
2. Urządzenie wysyła zaszyfrowane ramki `{channel, device_api_key, nonce, payload, tag}` (payload = hex ciphertext JSON przez AES-128-GCM; tag = 16 bajtowy znacznik uwierzytelniający).
3. Serwer deszyfruje, waliduje, aktualizuje `device_last_seen` i broadcastuje uproszczone dane w plaintext do frontendów.
4. Frontend wysyła komendy (np. światło, klima) w plaintext; serwer szyfruje per‑urządzenie przed wysyłką.
5. Dane okresowe lub zdarzeniowe zapisywane są przez endpointy PHP do PostgreSQL.

## 2. Decyzje projektowe (skrót) {#pl-readme-sec-2-decyzje}
| Aspekt | Wybór | Uzasadnienie |
|--------|-------|--------------|
| API | Cienkie endpointy PHP | Szybka iteracja, niski narzut |
| Real-time | Dedykowany Bun WS | Separacja logiki czasu rzeczywistego od warstwy SQL/PHP |
| Szyfrowanie | AES-128-GCM (tylko urządzenia) | Poufność + integralność, klucze nie trafiają do przeglądarki |
| Baza | PostgreSQL | JSONB + relacje |
| Frontend | Vue 3 + Pinia | Reaktywność, prostota |
| Format wiadomości | JSON w kopercie szyfrowanej | Łatwe debugowanie, elastyczność |

## 3. Struktura Repozytorium {#pl-readme-sec-3-struktura}
```
backend/      PHP + Caddy
jsServer/     Serwer WebSocket (Bun)
frontend/     Aplikacja Vue
IOT/ino/      Szkice ESP32
IOT/My_AES/   Biblioteka AES-128-GCM (mbedtls)
IOT/My_WiFi/  Prosty menedżer WiFi
postgreSQL/   Schemat + seed
docs/         Dokumenty
```

## 4. Schemat Bazy (skrót) {#pl-readme-sec-4-schemat-bazy}
Tabela | Cel | Kluczowe kolumny
------|-----|------------------
`devices` | Rejestr urządzeń | api_key, encryption_key, last_seen
`room_statistics` | Telemetria środowiska | temperature, humidity, pressure
`door_status` | Log zdarzeń drzwi | status_name, status_date
`profiles` | Profile konfiguracyjne | profile_json (JSONB)
`blinds_config` | Konfiguracja rolet | min_lux, max_lux, automate

## 5. Kanały WebSocket {#pl-readme-sec-5-kanaly-ws}
Kanał | Urządzenie → Serwer (zaszyfrowane) | Serwer → Frontend (plaintext)
------|------------------------------------|--------------------------------
`door_sensor` | `{doorOpen}` | `{channel, payload:{doorOpen}}`
`room_stats` | `{temperature, humidity, pressure}` | `{channel, temperature, humidity, pressure}`
`lux_sensor` | `{lux}` | `{channel, lux}`
`main_lights` | `{lightON}` | `{channel, lightON}`
`air_conditioning` | `{requestedTemp, function, klimaON, manualOverride}` | `{channel, payload:{... , currentTemp}}`

## 6. Szyfrowanie (AES-128-GCM) {#pl-readme-sec-6-szyfrowanie}
Pole | Opis
-----|-----
nonce | 12‑bajtowy losowy (24 hex)
tag | 16‑bajtowy znacznik (32 hex)
payload | Ciphertext JSON (hex)
alg | Stała "AES-128-GCM" (opcjonalne)

## 7. Endpointy (przykłady) {#pl-readme-sec-7-endpointy}
- `saveRoomStats.php` POST `{temperature,humidity,pressure}` → `{success:true}`
- `getDoorStatus.php` GET → historia zdarzeń
- `saveBlindsConfig.php` POST walidacja `min_lux < max_lux`
- `chooseProfile.php` GET (po id / name / wszystkie)
- `tuyaBlindsApi.php` GET `?action=open|close|status`

## 8. Firmware (skrót) {#pl-readme-sec-8-firmware}
Moduł | Rola
------|-----
`AESCrypto` | GCM encrypt/decrypt, nonce, tag
`MyWiFi` | Nadzór połączenia WiFi
Szkice | Kanały WebSocket + logika lokalna (histereza klimatyzacji, debounce, itp.)

## 9. Profile {#pl-readme-sec-9-profile}
JSON w kolumnie `profiles.profile_json` agreguje ustawienia: AC, WLED, rolety, światła.

## 10. Ulepszenia przyszłe {#pl-readme-sec-10-ulepszenia}
- AAD w GCM (kanał + api_key)
- Wykrywanie reuse nonce
- Logowanie strukturalne
- Rotacja kluczy + polityka

(Wersja pełna dokumentacji i tłumaczenie payloadów w osobnych plikach.)

## 11. Szybki start: Symulacja urządzenia {#pl-readme-sec-11-symulacja}
Możesz zasymulować urządzenie i wysłać pojedynczą zaszyfrowaną ramkę:
```
cd jsServer
bun run testDeviceSim.js room_stats <device_api_key> <16znakowyKlucz>
```
Kanały obsługiwane: `room_stats`, `door_sensor`, `lux_sensor`, `main_lights`, `air_conditioning`.
Skrypt:
- generuje nonce 12B
- szyfruje JSON przez AES-128-GCM
- wysyła identyfikację + ramkę
- wypisuje odpowiedzi (np. push temperatury dla klimatyzacji)

## 12. Rozwiązywanie problemów (Troubleshooting) {#pl-readme-sec-12-troubleshooting}
Objaw | Przyczyna | Rozwiązanie
----- | -------- | -----------
Brak danych w frontendzie | Błędny `device_api_key` | Sprawdź DB `devices` i klucz w szkicu
Decrypt failure w logu serwera | Zły klucz lub uszkodzony payload | Zweryfikuj długość klucza (≥16) i brak edycji hex
Brak temperatury w AC | Jeszcze nie przyszła ramka `room_stats` | Poczekaj na pierwszą publikację
Door status spam | Brak debounce w kliencie | Użyj logiki `enqueueStatus()`
Save blinds 400 | `min_lux >= max_lux` | Skoryguj wartości i ponów

## 13. Konserwacja / Utrzymanie {#pl-readme-sec-13-konserwacja}
Zadanie | Częstotliwość
------- | ------------
Rotacja kluczy urządzeń | Kwartalnie
Przegląd logów decrypt | Tygodniowo
Porządki w tabelach historycznych | Miesięcznie
Aktualizacja zależności / kontenerów | Kwartalnie lub wg potrzeb
Weryfikacja profili (nadmiar) | Kwartalnie

