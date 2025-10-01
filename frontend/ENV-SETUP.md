# Environment Variables Setup

This project now uses environment variables for managing URLs and API endpoints. The primary file is `.env.links` which contains all links previously hardcoded in the Pinia stores.

## Available Environment Variables

- `VITE_DATABASE_API` - Base URL for PHP API endpoints
- `VITE_CDN_URL` - Base URL for CDN/static assets
- `VITE_WLED_IP` - WLED device IP address
- `VITE_WLED_PRESETS_ENDPOINT` - Endpoint for WLED presets
- `VITE_WLED_STATE_ENDPOINT` - Endpoint for WLED state management
- `VITE_WS_URL` - WebSocket URL for real-time communication
- `VITE_DOOR_STATUS_API` - Door status API endpoint
- `VITE_ROOM_STATS_API` - Room statistics API endpoint

## How to Modify

You can update the URLs in the `.env.links` file without needing to modify the codebase. This makes it easy to change service locations for different environments.

## Development vs Production

The `.env.links` file will be loaded in all environments. If you need environment-specific configurations:

1. Create `.env.development` or `.env.production` files
2. Override any variables from `.env.links` as needed

## Docker Deployment

When deploying with Docker, you can override these environment variables using Docker's environment variable functionality.

```yaml
# Example in docker-compose.yml
services:
  frontend:
    # ...
    environment:
      - VITE_DATABASE_API=http://new-api-url:8884/
      - VITE_CDN_URL=http://new-cdn-url:8885/
```