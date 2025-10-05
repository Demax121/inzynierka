# Techniczna Dokumentacja (Dossier) – PL {#pl-techniczna-dokumentacja}

## Cel {#pl-cel}
Zbiorcza dokumentacja inżynierska obejmująca strukturę kodu, komponenty, protokoły, decyzje architektoniczne oraz wskazówki utrzymaniowe.

## 1. Struktura Repozytorium {#pl-sec-1-struktura}
Folder | Opis
------|-----
backend/ | Endpointy PHP + konfiguracja Caddy
jsServer/ | Serwer WebSocket (Bun, kanały, szyfrowanie)
frontend/ | Aplikacja Vue (Pinia, komponenty, style)
IOT/ino/ | Szkice ESP32 (po jednym na urządzenie)
IOT/My_AES/ | Implementacja AES-128-GCM (mbedtls)
IOT/My_WiFi/ | Lekka klasa zarządzania WiFi
postgreSQL/ | Schemat bazy + seed
docs/ | Dokumentacja (EN/PL, payloady, spisy)

## 2. Baza Danych (init.sql) {#pl-sec-2-baza-danych}
Tabela | Kolumny Kluczowe | Uwagi
------|------------------|------
`devices` | api_key, encryption_key, last_seen | Identyfikacja i klucze szyfrowania
`room_statistics` | temperature, humidity, pressure, stat_date | Telemetria środowiskowa
`door_status` | status_name, status_date | Historia otwarć / zamknięć
`profiles` | profile_name, profile_json (JSONB) | Profile złożone multi‑kanałowe
`blinds_config` | min_lux, max_lux, automate | Pojedyncza konfiguracja progowa rolet

ER (ASCII):
```
 devices --(loose)--> room_statistics
 devices --(loose)--> door_status
 profiles (JSONB blob) unrelated FKs
 blinds_config (singleton row)
```
Brak kluczy obcych – uproszczenie przepływu rejestracji urządzeń.

## 3. Kanały WebSocket {#pl-sec-3-kanaly-ws}
Kanał | Treść urządzenie→serwer (GCM) | Broadcast frontend
------|-------------------------------|------------------
room_stats | {temperature, humidity, pressure} | {temperature, humidity, pressure}
door_sensor | {doorOpen} | {payload:{doorOpen}}
lux_sensor | {lux} | {lux}
main_lights | {lightON} | {lightON}
air_conditioning | {requestedTemp,function,klimaON,manualOverride} | {payload:{...,currentTemp}}

## 4. Szyfrowanie {#pl-sec-4-szyfrowanie}
- AES-128-GCM
- Pola: `nonce` (12B), `payload` (hex), `tag` (16B), opcj. `alg`
- Per‑device statyczny klucz (pierwsze 16 znaków w DB)
- Serwer odświeża mapę kluczy co 5 min
- Brak AAD (można rozszerzyć o kanał + api_key)

## 5. Endpointy PHP (skrót) {#pl-sec-5-endpointy-php}
Plik | Rola | Walidacja
-----|-----|---------
getBlindsConfig.php | Odczyt konfiguracji rolet | GET
saveBlindsConfig.php | Zapis progów + automate | POST + min<max
setBlindsAutomate.php | Przełączanie trybu automatycznego | POST
saveDoorStatus.php | Zdarzenia drzwi | POST (status enum)
getDoorStatus.php | Historia drzwi | GET
saveRoomStats.php | Zapis statystyk | POST (numeryczne pola)
chooseProfile.php | Odczyt profili | GET (id/name)
createProfile.php | Utworzenie profilu | POST JSONB
deleteProfile.php | Usunięcie profilu | POST id
tuyaBlindsApi.php | Proxy Tuya | GET action
getEncryptionKeys.php | Lista kluczy | GET
updateDeviceStatus.php | Aktualizacja last_seen | POST api_key

## 6. Firmware – Wzorzec {#pl-sec-6-firmware-wzorzec}
Etap | Opis
----|-----
WiFi | MyWiFi::connect() blokująco, potem loop()
WS | Identyfikacja `{type:"esp32_identification"}`
Kryptografia | Generacja nonce, GCM encrypt, wysyłka
Logika | Histereza (klima), odczyt sensorów, debounce (jeśli potrzebne)
Odbiór | Decrypt + weryfikacja tagu, aktualizacja stanu

## 7. AESCrypto (GCM) {#pl-sec-7-aescrypto}
Metoda | Cel
-------|----
`generateNonce()` | 12‑bajtowy hex nonce
`encrypt(plain, nonce, cipherOut, tagOut)` | AEAD encrypt → bool
`decrypt(cipherHex, nonceHex, tagHex)` | Zwraca plaintext lub ""
Implementacja: mbedtls_gcm_* (auth decrypt), hex narzędzia pomocnicze.

## 8. MyWiFi {#pl-sec-8-mywifi}
Funkcja | Opis
--------|-----
`connect()` | Pierwsze połączenie (timeout ~15s)
`loop()` | Okresowy reconnect
`isConnected()` | Status
Uwaga: hasła w kodzie – do produkcji przenieść do NVS / provisioning.

## 9. Serwer Bun – Logika Kanałów {#pl-sec-9-serwer-bun}
Kanał | Specyfika
------|----------
room_stats | Aktualizuje cache temperatury + push do AC
air_conditioning | Łączy stan z `currentTemp` dla frontendu
main_lights | Rozróżnia komendy (frontend) i status (ESP32)
door_sensor | Prosty broadcast
lux_sensor | Broadcast + potencjalna baza pod automatyzację rolet
Ping | Co 10s – każdy ESP32 ma odpowiedzieć stanem

## 10. Frontend – Kluczowe Komponenty {#pl-sec-10-frontend-komponenty}
Komponent | Rola
---------|-----
roomStatsCard | Prezentacja temperatury/wilgotności/ciśnienia
mainLightsCard | Sterowanie światłem + odzwierciedlenie stanu
airConditioningCard | Zmiana `requestedTemp`, manual toggle + tryb
luxSensorCard | Odczyt lux
blindsAutomateCard | Konfiguracja automatu rolet
blindsCard | Sterowanie otwórz/zamknij (Tuya)
chooseProfileCard | Aplikowanie profilów
createProfileCard | Tworzenie profilu
wledCard | Ustawienia WLED

## 11. Profile {#pl-sec-11-profile}
Przykład JSON (fragment):
```json
{
  "AC": {"channel":"air_conditioning","payload":{"requestedTemp":24}},
  "lights": {"channel":"main_lights","payload":{"state":true}},
  "blinds": {"state":"open"},
  "WLED": {"on":false, "preset_name":"Wyłączony"}
}
```
Interpretacja frontendu → sekwencyjne wysłanie właściwych ramek/żądań.

## 12. Testowanie i Symulacja {#pl-sec-12-testowanie-symulacja}
Szybki start:
```
cd jsServer
bun run testDeviceSim.js door_sensor <device_api_key> <16charKey>
```
Zakres kanałów jak wyżej. Modyfikacja hex w `payload`/`tag` = odrzucona ramka.

## 13. Bezpieczeństwo – Uzupełnienia {#pl-sec-13-bezpieczenstwo-uzupelnienia}
Ryzyko | Mitigacja
-------|----------
Podsłuch LAN | GCM szyfruje treść urządzeń
Modyfikacja | Tag uwierzytelnia
Utrata klucza | Rotacja + restart serwera (refresh mapy)
Replay | (Do wdrożenia) cache nonce
Frontend exploit | Brak kluczy po stronie UI

## 14. Ulepszenia Proponowane {#pl-sec-14-ulepszenia-proponowane}
- AAD (kanał + api_key)
- Replay detection
- Logowanie strukturalne JSON
- Indeksy (room_statistics.stat_date DESC)
- Toasty UX

## 15. Checklista Utrzymaniowa {#pl-sec-15-checklista-utrzymaniowa}
Zadanie | Interwał
-------|---------
Rotacja kluczy | Kwartalnie
Analiza decrypt fail | Tygodniowo
Pruning danych historycznych | Miesięcznie
Aktualizacja zależności | Kwartalnie
Przegląd profili | Kwartalnie

## 16. Argumentacja Technologiczna {#pl-sec-16-argumentacja-technologiczna}
Wybór | Powód | Alternatywa Odrzucona
------|-------|---------------------
Bun WS | Minimalny narzut, prostota | Integracja WS w PHP (złożone)
AES-128-GCM | Integr., wydajność, prostota | CBC (brak integralności)
JSONB profile | Elastyczność | Tabele z wieloma FK (sztywne)
Vue 3 + Pinia | Reaktywność + ergonomia | React (cięższy dla skali)

## 17. Wnioski {#pl-sec-17-wnioski}
Projekt łączy prostotę implementacyjną z rozsądnym modelem bezpieczeństwa (AEAD między urządzeniem a serwerem). Rozwojowo najważniejsze: wprowadzenie AAD, replay protection i warstwy obserwowalności.

---
Dokument dołączony do pracy inżynierskiej – aktualizować przy istotnych refaktorach.
