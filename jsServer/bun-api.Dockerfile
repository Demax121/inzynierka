# Use Bun official image
FROM oven/bun:latest

# Set working directory
WORKDIR /app

# Copy package files and install dependencies
COPY package.json ./
RUN bun install

# Copy the rest of the source code
COPY . .

CMD ["bun", "bunServer.js"]