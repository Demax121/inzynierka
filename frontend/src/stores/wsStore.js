import { defineStore } from 'pinia'

function normalizeToWs(raw) {
  if (!raw || typeof raw !== 'string') return raw;
  const forceWss = import.meta.env.VITE_FORCE_WSS === 'true';
  const pageIsHttps = typeof window !== 'undefined' && window.location?.protocol === 'https:';
  const wantSecure = forceWss || pageIsHttps;
  const trimmed = raw.trim();
  const lower = trimmed.toLowerCase();
  if (lower.startsWith('ws://') || lower.startsWith('wss://')) {
    if (wantSecure && lower.startsWith('ws://')) return 'wss://' + trimmed.slice(5);
    return trimmed;
  }
  if (lower.startsWith('http://') || lower.startsWith('https://')) {
    return (wantSecure ? 'wss://' : 'ws://') + trimmed.replace(/^https?:\/\//i, '');
  }
  return (wantSecure ? 'wss://' : 'ws://') + trimmed.replace(/^\/+/, '');
}

function resolveWsUrl() {
  const injected = typeof window !== 'undefined' && window.__APP_CFG && window.__APP_CFG.wsUrl;
  const envUrl = import.meta.env.VITE_WS_URL_PREFIX;
  const base = injected || envUrl;
  if (!base) {
    console.warn('[wsStore] Missing VITE_WS_URL_PREFIX and no window.__APP_CFG.wsUrl override provided');
    return base;
  }
  return normalizeToWs(base);
}

export const useWsStore = defineStore('ws', {
  state: () => ({
    wsUrl: resolveWsUrl(),
  })
})