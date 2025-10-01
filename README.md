# Project Structure & Overview

This repository contains a complete stack for a smart home automation system. Below is a breakdown of each folder and its purpose, along with key files and their roles.

---

## Folder & File Structure

```
Praca inzynierska/
│
├── backend/
│   ├── php/
│   │   ├── config.php
│   │   ├── chooseProfile.php
│   │   ├── getBlindsConfig.php
│   │   ├── tuyaBlindsApi.php
│   │   └── ...other PHP files
│   ├── caddy-server.Dockerfile
│   └── ...other backend files
│
├── jsServer/
│   ├── bunServer.js
│   ├── bun-api.Dockerfile
│   └── ...other JS server files
│
├── frontend/
│   ├── src/
│   │   ├── main.js
│   │   ├── components/
│   │   │   └── Navbar_component.vue
│   │   ├── stores/
│   │   │   ├── linkStore.js
│   │   │   └── saveStatsStore.js
│   │   └── ...other frontend files
│   ├── index.html
│   ├── vite.config.js
│   ├── frontend.Dockerfile
│   └── ...other frontend files
│
├── CDN/
│   ├── Caddyfile
│   ├── caddy-cdn.Dockerfile
│   └── ...static assets (images, config_files, etc.)
│
├── IOT/
│   ├── doorSensor.ino
│   ├── acController.ino
│   ├── pliki/
│   │   └── JSON format.txt
│   └── ...other microcontroller code
│
├── inżynierka plan.txt
├── README.md
└── ...other files
```

---

## Folder Details

### `backend/`
Contains all server-side logic for database and API operations.

- **php/**: PHP scripts for database access, API endpoints, and integration with external services (e.g., Tuya blinds).
  - `config.php`: Database and Tuya API credentials/configuration.
  - `chooseProfile.php`: API for retrieving user profiles from the database.
  - `getBlindsConfig.php`: API for fetching blinds automation configuration.
  - `tuyaBlindsApi.php`: Handles communication with Tuya cloud API for blinds control.
- **caddy-server.Dockerfile**: Dockerfile for running the backend PHP server with Caddy.

### `jsServer/`
Node.js/Bun-based WebSocket server for real-time communication between frontend, ESP32 devices, and automation logic.

- **bunServer.js**: Main WebSocket server. Handles device identification, message routing between frontend and ESP32 devices, and automation logic for sensors, lights, air conditioning, and more.
- **bun-api.Dockerfile**: Dockerfile for running the JS server with Bun.

### `frontend/`
Vue 3 SPA for user interaction and control.

- **src/**: Source code for the frontend app.
  - `main.js`: Entry point for the Vue app.
  - `components/`: Vue components (e.g., navigation bar, cards).
  - `stores/`: Pinia stores for state management (API links, stats saving, etc.).
    - `linkStore.js`: Centralized management of API endpoints and URLs.
    - `saveStatsStore.js`: Handles periodic saving of room stats to the database.
- **index.html**: Main HTML file for the SPA.
- **vite.config.js**: Vite configuration for building and serving the frontend.
- **frontend.Dockerfile**: Dockerfile for running the frontend with Bun.

### `CDN/`
Static file server for images and configuration files.

- **Caddyfile**: Configuration for Caddy to serve static assets.
- **caddy-cdn.Dockerfile**: Dockerfile for running the CDN server.
- **assets/**: Directory for images and config files, served via CDN.

### `IOT/`
Firmware and code for ESP32 microcontrollers.

- **doorSensor.ino**: Firmware for the door sensor device (detects open/close state, communicates via WebSocket).
- **acController.ino**: Firmware for the air conditioning controller (displays status, handles temperature control, communicates via WebSocket).
- **pliki/JSON format.txt**: Documentation of JSON message formats used for device-server communication.

### Other Files

- **inżynierka plan.txt**: Project planning notes, feature requirements, and JSON profile format specification.
- **README.md**: This documentation file.

---

## Database

- **Database name**: `inzynierka`
- **Used by**: Backend PHP scripts for storing profiles, blinds configuration, and room stats.
- **Connection details**: See `backend/php/config.php` for credentials.

---

## Key Components

### `bunServer.js`
- Real-time WebSocket server for device and frontend communication.
- Handles device identification, message routing, automation logic, and periodic pings to ESP32 devices.
- Channels supported: `door_sensor`, `room_stats`, `main_lights`, `lux_sensor`, `air_conditioning`.

### `CDN/`
- Serves static assets (images, config files) via HTTP using Caddy.
- Used by frontend for loading images and configuration files.

### `IOT/`
- Contains ESP32 firmware for sensors and controllers.
- Defines JSON formats for communication with the server.

---

## Database Access

### Access pgAdmin

Open your browser and go to:
```
http://localhost:8882
```

#### Login
- **Email**: `admin@admin.com`
- **Password**: `admin123`

#### Add New Server Connection

- **Host name/address**: `postgres` (Docker service name)
- **Port**: `5432`
- **Maintenance database**: `inzynierka`
- **Username**: `postgresAdmin`
- **Password**: `postgres123`

**Note**: Use `postgres` as the hostname and port `5432` for internal Docker network communication.

---

## Production Deployment

### Domain Structure
The application uses a subdomain-based architecture with **simplysmart.duckdns.org**:

- **Main App**: `https://simplysmart.duckdns.org` → Vue.js frontend
- **API**: `https://api.simplysmart.duckdns.org` → PHP backend APIs
- **CDN**: `https://cdn.simplysmart.duckdns.org` → Static assets
- **WebSocket**: `wss://ws.simplysmart.duckdns.org` → Real-time server
- **Admin**: `https://admin.simplysmart.duckdns.org` → pgAdmin interface

### Reverse Proxy
Uses **Nginx Proxy Manager** for:
- SSL certificate management (Let's Encrypt)
- Subdomain routing
- WebSocket support
- Centralized proxy configuration

See `NGINX_PROXY_MANAGER_SETUP.md` for detailed configuration instructions.

---

## Additional Notes

- All services are containerized for easy deployment.
- The stack includes backend APIs, real-time JS server, Vue SPA frontend, CDN for static assets, and ESP32 firmware.
- Profiles and automation logic are stored in the PostgreSQL database `inzynierka`.
- Production uses Nginx Proxy Manager with SSL certificates for all subdomains.
