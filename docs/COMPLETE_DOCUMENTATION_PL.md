# Kompletna Dokumentacja Projektu (PL) {#pl-kompletna-dokumentacja}

## Spis Treści {#pl-spis-tresci}
1. Streszczenie
2. Architektura
3. Model Danych i ERD
4. Przepływy Danych Runtime
5. Protokół Komunikacyjny (WebSocket + Szyfrowanie)
6. Backend (Endpointy PHP)
7. Serwer Czasu Rzeczywistego (Bun WS Hub)
8. Frontend (Vue + Pinia)
9. Firmware (ESP32)
10. Biblioteki Własne (AESCrypto, MyWiFi)
11. Model Bezpieczeństwa i Wybór Kryptografii
12. System Profili
13. Obsługa Błędów i Odporność
14. Wydajność
15. Workflow Deweloperski
16. Katalog Komponentów (Vue)
17. Plan Ulepszeń
18. Glosariusz
19. Lista Utrzymaniowa
 20. Symulacja Urządzenia (Quickstart)

---
## 1. Streszczenie {#pl-sec-1-streszczenie}
System inteligentnego pomieszczenia z szyfrowaną telemetrią i sterowaniem, oparty na lekkim stosie: PostgreSQL + PHP (cienkie endpointy) + Bun WebSocket + Vue 3 + szkice ESP32. Szyfrowanie AES-128-GCM między urządzeniem a serwerem zwiększa poufność i integralność bez nadmiernej złożoności.

## 2. Architektura {#pl-sec-2-architektura}
Warstwa | Rola | Technologia
--------|------|------------
Urządzenia | Pomiar i sterowanie | ESP32 (Arduino C++), mbedTLS
Transport | Dwukierunkowy realtime | WebSocket (Bun)
API | Trwałość i konfiguracja | PHP + PDO
Baza | Dane trwałe + JSONB | PostgreSQL
Frontend | Dashboard i akcje | Vue 3 + Pinia + Vite
Bezpieczeństwo | Szyfrowanie AEAD | AES-128-GCM

## 3. Model Danych (ERD) {#pl-sec-3-model-danych-erd}
```
+---------------+        +-----------------+
|   devices     |        | room_statistics |
|---------------|        |-----------------|
| device_id PK  |   +--> | stat_id PK      |
| device_api_key|   |    | stat_date       |
| device_encryption_key| | stat_temperature|
| device_last_seen     | | stat_humidity   |
+---------------+   |    | stat_pressure   |
                    |
+---------------+   |    +-----------------+
| door_status   |   |
|---------------|   |    +-----------------+
| status_id PK  |   |    |  profiles        |
| status_date   |   |    |------------------|
| status_name   |   |    | profile_id PK    |
+---------------+   |    | profile_name     |
                    |    | profile_json JSONB|
               +------------------+
               | blinds_config    |
               |------------------|
               | ID_config PK     |
               | min_lux          |
               | max_lux          |
               | automate         |
               +------------------+
```

## 4. Przepływy Danych {#pl-sec-4-przeplywy-danych}
Scenariusz | Opis
-----------|-----
Telemetria | Urządzenie szyfruje → serwer deszyfruje → frontend plaintext → push temp do AC
Światło (komenda) | Frontend plaintext → serwer szyfruje → sterownik → status zwrotny
Drzwi | Szyfrowane doorOpen → broadcast → debounced zapis door_status
Profil | Frontend aplikuje → wysyła zestaw akcji (WS/HTTP) → urządzenia aktualizują stan

## 5. Protokół Komunikacyjny {#pl-sec-5-protokol-komunikacyjny}
Koperta urządzenia (szyfrowane): `{channel, device_api_key, nonce, payload, tag, alg}`
Broadcast frontend: uproszczone JSON bez pól kryptograficznych.

## 6. Endpointy PHP {#pl-sec-6-endpointy-php}
Wzorce: walidacja metody, JSON parse, `success/error`, prepared statements.
Kluczowe: statystyki, drzwi, rolety (konfiguracja + Tuya), profile (CRUD), klucze szyfrowania, aktualizacja last_seen.

## 7. Serwer WS (Bun) {#pl-sec-7-serwer-ws-bun}
- Mapa klientów z typem (frontend/esp32)
- Odświeżanie kluczy co 5 min
- Ping urządzeń co 10 s
- Szyfrowanie per urządzenie (komendy + push temperatury)

## 8. Frontend {#pl-sec-8-frontend}
- Pinia stores: wsStore, doorStatusStore (debounce), saveStatsStore (interwał), automateStore
- Komponenty kart: per kanał + WLED + profile + rolety
- Reconnect: prosty backoff 5 s

## 9. Firmware ESP32 {#pl-sec-9-firmware-esp32}
- MyWiFi: inicjalizacja i watchdog WiFi
- AESCrypto: GCM (nonce + tag)
- Szkice: identyfikacja, telemetria, histereza klimatyzacji, natychmiastowa ocena trybu przy manualnym włączeniu

## 10. Biblioteki Własne {#pl-sec-10-biblioteki-wlasne}
Biblioteka | Powód | Uwaga
-----------|-------|------
AESCrypto | Abstrakcja mbedTLS + migracja na GCM | Nonce 12B, tag 16B
MyWiFi | Redukcja powielania kodu łączenia | Do rozbudowy (provisioning)

## 11. Bezpieczeństwo {#pl-sec-11-bezpieczenstwo}
Zalety GCM: poufność + integralność, brak paddingu, wsparcie sprzętowe. Brak jeszcze kontroli replay (plan: cache nonce). Brak AAD – można dodać (kanał+api_key) do dodatkowego związania kontekstu.

## 12. System Profili {#pl-sec-12-system-profili}
JSONB w `profiles` przechowuje konfiguracje wielu subsystemów. Aplikowanie: sekwencyjne wysłanie per‑kanał.

## 13. Odporność {#pl-sec-13-odpornosc}
Mechanizm | Technika
----------|---------
WiFi | Rekonfiguracja w loop()
WebSocket | Ping + reconnect
Door burst | Debounce
Rotacja kluczy | Okresowe odświeżenie mapy

## 14. Wydajność {#pl-sec-14-wydajnosc}
Mała liczba urządzeń → pętla broadcast wystarczająca. Optymalizacja: indeksy, mapy subskrypcji. GCM narzut znikomy dla małych ramek.

## 15. Workflow {#pl-sec-15-workflow}
Docker compose + lokalne Bun/Vite dev. Symulator `testDeviceSim` do testów szyfrowania.

### 15.1 Symulacja Urządzenia (Quickstart) {#pl-sec-15-1-symulacja}
```
cd jsServer
bun run testDeviceSim.js room_stats <device_api_key> <16znakowyKlucz>
```
Kanały: `room_stats`, `door_sensor`, `lux_sensor`, `main_lights`, `air_conditioning`.
Skrypt:
1. Wysyła identyfikację
2. Generuje nonce 12B i szyfruje przykładowy JSON AES-128-GCM
3. Wysyła ramkę i loguje odpowiedzi serwera
Test integralności: zmień 1 znak w `payload` lub `tag` – deszyfracja zostanie odrzucona.

## 16. Komponenty (Vue) {#pl-sec-16-komponenty-vue}
Patrz katalog – każdy odwzorowuje pojedynczy kanał lub funkcję (profile, WLED, rolety). Ujednolicone SCSS BEM.

## 17. Plan Ulepszeń {#pl-sec-17-plan-ulepszen}
- AAD w GCM
- Wykrywanie replay (nonce cache)
- Logowanie strukturalne
- Mapy subskrypcji
- Powiadomienia UI

## 18. Glosariusz {#pl-sec-18-glosariusz}
Termin | Znaczenie
------|-----------
Nonce | Jednorazowa wartość losowa
Profil | Migawka konfiguracji multi‑kanałowej
Histereza | Pasmo zapobiegające flappingowi klimatyzacji

## 19. Utrzymanie {#pl-sec-19-utrzymanie}
Zadanie | Częstotliwość
--------|--------------
Rotacja kluczy | Kwartalnie
Analiza logów decrypt | Tygodniowo
Pruning history tables | Miesięcznie
Przegląd profili | Kwartalnie

---
Dokument generowany automatycznie; aktualizować wraz ze zmianami kodu.

## 20. Symulacja Urządzenia (Quickstart) {#pl-sec-20-symulacja}
Skrót (patrz sekcja 15.1):
```
cd jsServer
bun run testDeviceSim.js main_lights <device_api_key> <16znakowyKlucz>
```
Argument pierwszy to kanał.
