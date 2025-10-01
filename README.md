# Offline Inżynierka - Quick Start (pgAdmin)

This project ships a local PostgreSQL database plus pgAdmin via Docker Compose.
Below are the steps and defaults to connect to the database from pgAdmin.

## Start the stack

Make sure Docker Desktop is running, then start the services:

- Start all services: `docker compose up -d`
- Stop all services: `docker compose down`

The compose file exposes pgAdmin at http://localhost:8882.

## pgAdmin login

Open your browser and go to:

- URL: http://localhost:8882
- Email: admin@admin.com
- Password: admin123

(These values come from the `.env` file and can be changed there.)

## Create a new server in pgAdmin

Inside pgAdmin (left sidebar → Servers → Register → Server):

- General tab:
  - Name: Inzynierka_db (any name is fine)
- Connection tab:
  - Host name/address: offline_postgres_inzynierka
  - Port: 5432
  - Maintenance database: inzynierka
  - Username: postgresAdmin
  - Password: postgres123
  - Save Password: checked

Click Save. You should now see the database and schemas on the left.

## Where these values come from

All values are defined in the repository `.env` file and used by Docker Compose:

- POSTGRES_USER=postgresAdmin
- POSTGRES_PASSWORD=postgres123
- POSTGRES_DB=inzynierka
- PGADMIN_EMAIL=admin@admin.com
- PGADMIN_PASSWORD=admin123
- pgAdmin is served on host port 8882 → container port 80

The Postgres container is reachable by other containers via the internal Docker hostname `offline_postgres_inzynierka` on port 5432.

## Troubleshooting

- If pgAdmin doesn’t load, ensure the container is running: `docker ps` and look for `offline_pgadmin_inzynierka`.
- If you cannot connect from pgAdmin:
  - Verify the Postgres container is running: `offline_postgres_inzynierka`.
  - Confirm credentials in `.env` match the values you entered.
  - If you previously started the stack with different credentials, you may need to remove the Postgres volume to reinitialize the DB (this will wipe data):
    - `docker compose down`
    - `docker volume rm offlineinzynierka_offline_postgres_data`
    - `docker compose up -d`

## Ports summary

- pgAdmin (web UI): http://localhost:8882
- Postgres (inside Docker network): offline_postgres_inzynierka:5432

