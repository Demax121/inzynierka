# Use Bun official image
FROM oven/bun:latest

# Set working directory
WORKDIR /app

ARG PORT_REFERENCE
ARG HOST_REFERENCE

# Copy package files and install deps early (for caching)
COPY package.json ./
COPY bun.lock ./

RUN bun install

# Copy rest of the source
COPY . .

# Set environment vars (can also be in docker-compose)
ENV VITE_PORT=$PORT_REFERENCE
ENV VITE_HOST=${HOST_REFERENCE}

# Run Vite dev server
CMD ["bun", "run", "dev"]
