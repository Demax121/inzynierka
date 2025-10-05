FROM php:8.2-fpm

ARG POSTGRES_USER
ARG POSTGRES_PASSWORD
ARG POSTGRES_DB
# These ARGs map from unified DB_* variables in docker-compose (passed as POSTGRES_* here for consistency with upstream env names)

ENV POSTGRES_USER=$POSTGRES_USER
ENV POSTGRES_PASSWORD=$POSTGRES_PASSWORD
ENV POSTGRES_DB=$POSTGRES_DB

RUN apt-get update && apt-get install -y \
    libpq-dev \
    && docker-php-ext-install pdo pdo_pgsql

WORKDIR /var/www/php

CMD ["php-fpm"]