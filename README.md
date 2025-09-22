# Connecting to database

## Access pgAdmin
Open your browser and go to:
```
http://localhost:8882
```

## Login to pgAdmin
- **Email**: `admin@admin.com`
- **Password**: `admin123`

## Add New Server Connection
Once logged in, right-click "Servers" in the left panel and select "Create > Server..."

### General Tab
- **Name**: `PostgreSQL Local` (or any name you prefer)

### Connection Tab
- **Host name/address**: `postgres` (service name from docker-compose.yml)
- **Port**: `5432`
- **Maintenance database**: `basePostgresDB`
- **Username**: `postgresAdmin`
- **Password**: `postgres123`

## Important Notes
- Use `postgres` as the hostname (this is the Docker service name)
- Use the native port `5432`, not the exposed port `8881`
- The containers communicate internally using Docker's network