FROM caddy:latest

COPY Caddyfile /etc/caddy/Caddyfile

WORKDIR /srv/assets
