// Simple encrypted device message simulator
// Usage: bun run testDeviceSim.js <channel> <apiKey> <encryptionKey16>
// Example: bun run testDeviceSim.js main_lights kZ8UQmdrDar8 Vfyu3xT6e6yy79iE

import WebSocket from 'ws';
import * as CryptoJS from 'crypto-js';

const [,, channel, apiKey, encKey] = process.argv;
if (!channel || !apiKey || !encKey) {
  console.error('Args: <channel> <device_api_key> <device_encryption_key16>');
  process.exit(1);
}
if (encKey.length < 16) {
  console.error('Encryption key must be at least 16 chars');
  process.exit(1);
}

// Build a sample payload per channel
const samplePayloads = {
  door_sensor: { doorOpen: true },
  room_stats: { temperature: 23.4, humidity: 44.5, pressure: 1009.2 },
  lux_sensor: { lux: 777 },
  main_lights: { lightON: true },
  air_conditioning: { requestedTemp: 24, function: '', klimaON: false, manualOverride: false }
};

const payloadObj = samplePayloads[channel] || { test: true };

function encrypt(obj) {
  const iv = CryptoJS.lib.WordArray.random(16);
  const keyWA = CryptoJS.enc.Utf8.parse(encKey.slice(0,16));
  const cipher = CryptoJS.AES.encrypt(JSON.stringify(obj), keyWA, { iv, padding: CryptoJS.pad.Pkcs7, mode: CryptoJS.mode.CBC });
  return { msgIV: CryptoJS.enc.Hex.stringify(iv), payload: cipher.ciphertext.toString(CryptoJS.enc.Hex) };
}

const wsUrl = process.env.SIM_WS || 'ws://localhost:3000';
console.log('Connecting to', wsUrl);
const ws = new WebSocket(wsUrl);

ws.on('open', () => {
  console.log('Connected, sending identification and encrypted payload...');
  ws.send(JSON.stringify({ type: 'esp32_identification', channel, device_api_key: apiKey }));
  setTimeout(() => {
    const enc = encrypt(payloadObj);
    ws.send(JSON.stringify({ identity: channel + '_sim', channel, device_api_key: apiKey, ...enc }));
    console.log('Encrypted frame sent:', enc);
  }, 500);
});

ws.on('message', (data) => {
  try { console.log('RX:', JSON.parse(data.toString())); } catch { console.log('RX raw:', data.toString()); }
});

ws.on('close', () => console.log('Connection closed'));
ws.on('error', (e) => console.error('WS error', e));
