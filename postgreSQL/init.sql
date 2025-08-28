CREATE TABLE TEST (
    ID INTEGER,
    NAME VARCHAR(50)
);

INSERT INTO TEST (ID, NAME) VALUES (1, 'DB WORKS');

-- Device authentication table
CREATE TABLE devices (
    id SERIAL PRIMARY KEY,
    device_uid VARCHAR(100) UNIQUE NOT NULL,
    mac_address VARCHAR(17) NOT NULL,
    encryption_key VARCHAR(256) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Temporary keys table
CREATE TABLE temp_keys (
    id SERIAL PRIMARY KEY,
    temp_key VARCHAR(256) NOT NULL,
    device_request_hash VARCHAR(256),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP DEFAULT (CURRENT_TIMESTAMP + INTERVAL '10 minutes')
);

-- Room statistics table
CREATE TABLE roomStats (
    id SERIAL PRIMARY KEY,
    temperature DECIMAL(5,2),
    humidity DECIMAL(5,2),
    pressure DECIMAL(7,2),
    recorded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Insert sample device for testing
INSERT INTO devices (device_uid, mac_address, encryption_key) 
VALUES ('ESP32_001', 'AA:BB:CC:DD:EE:FF', 'permanent_encryption_key_123456789');