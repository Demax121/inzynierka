# Nginx Proxy Manager Subdomain Configuration

This document ### Docker Compose Port Mapping

The following internal container ports are used:
- Frontend: `vue_frontend_inzynierka:5173`
- Backend: `backend_server_caddy_inzynierka:80`
- CDN: `caddy-cdn_inzynierka:80`
- WebSocket: `bun_api_inzynierka:3000`
- pgAdmin: `pgadmin_ui_inzynierka:80`

### Network Configuration

Nginx Proxy Manager is connected to both networks:
- **`default` network**: For internet access (Let's Encrypt, external connections)
- **`inzynierka_network`**: For internal service communication

This dual-network setup ensures:
- ✅ SSL certificates can be obtained from Let's Encrypt
- ✅ Internal services are accessible for proxy forwarding
- ✅ Proper isolation of internal services the subdomain structure and Nginx Proxy Manager configuration for the SimplySmart home automation system.

## Domain Structure

Main domain: **simplysmart.duckdns.org**
Wildcard subdomain: **\*.simplysmart.duckdns.org**

## Subdomain Mapping

### 1. Main Frontend
- **Domain**: `simplysmart.duckdns.org`
- **Target**: `http://127.0.0.1:5173` (or `http://localhost:5173`)
- **SSL**: Let's Encrypt enabled
- **Purpose**: Vue.js frontend application

### 2. API Backend
- **Domain**: `api.simplysmart.duckdns.org`
- **Target**: `http://127.0.0.1:8080` (or `http://localhost:8080`)
- **SSL**: Let's Encrypt enabled
- **Purpose**: PHP backend APIs for database operations and external services

### 3. CDN Assets
- **Domain**: `cdn.simplysmart.duckdns.org`
- **Target**: `http://127.0.0.1:8081` (or `http://localhost:8081`)
- **SSL**: Let's Encrypt enabled
- **Purpose**: Static assets (images, config files)

### 4. WebSocket Server
- **Domain**: `ws.simplysmart.duckdns.org`
- **Target**: `http://127.0.0.1:3000` (or `http://localhost:3000`)
- **SSL**: Let's Encrypt enabled
- **WebSocket Support**: Enabled
- **Purpose**: Real-time communication with ESP32 devices

### 5. Database Admin
- **Domain**: `admin.simplysmart.duckdns.org`
- **Target**: `http://127.0.0.1:8882` (or `http://localhost:8882`)
- **SSL**: Let's Encrypt enabled
- **Purpose**: pgAdmin database management interface

## Nginx Proxy Manager Setup Steps

1. **Access Nginx Proxy Manager**
   - URL: `http://your-server-ip:81`
   - Default credentials: `admin@example.com` / `changeme`

2. **Add SSL Certificate**
   - Go to SSL Certificates
   - Add Let's Encrypt certificate for `*.simplysmart.duckdns.org`
   - Include domains: `simplysmart.duckdns.org` and `*.simplysmart.duckdns.org`

3. **Create Proxy Hosts**
   For each subdomain above:
   - Go to Proxy Hosts
   - Add Proxy Host
   - Enter domain name
   - Enter scheme (http) and forward hostname/IP
   - Enter forward port
   - Enable SSL and select the wildcard certificate
   - For WebSocket server: Enable "Websockets Support"

## Docker Compose Port Mapping

The following internal container ports are used:
- Frontend: `vue_frontend_inzynierka:5173`
- Backend: `backend_server_caddy_inzynierka:80`
- CDN: `caddy-cdn_inzynierka:80`
- WebSocket: `bun_api_inzynierka:3000`
- pgAdmin: `pgadmin_ui_inzynierka:80`

## Environment Variables Updated

### Frontend (.env.development)
```bash
VITE_BACKEND_URL_PREFIX=https://api.simplysmart.duckdns.org
VITE_CDN_URL_PREFIX=https://cdn.simplysmart.duckdns.org
VITE_WS_URL_PREFIX=wss://ws.simplysmart.duckdns.org
VITE_WLED_IP=https://192.168.1.25
```

## Benefits of This Setup

1. **Clean URLs**: Easy to remember and professional looking
2. **SSL Everywhere**: All subdomains secured with Let's Encrypt
3. **Service Isolation**: Each service has its own subdomain
4. **Scalability**: Easy to add new services with new subdomains
5. **Maintenance**: Centralized proxy management through web UI

## Notes

- Make sure to configure DuckDNS with the wildcard subdomain `*.simplysmart.duckdns.org`
- All services communicate internally via Docker network
- Only Nginx Proxy Manager exposes ports 80, 81, and 443 to the host
- WebSocket connections are properly handled with the WebSocket support enabled