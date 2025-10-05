import { defineStore } from 'pinia'

function resolveWsUrl() {
  const injected = typeof window !== 'undefined' && window.__APP_CFG && window.__APP_CFG.wsUrl;
  const envUrl = import.meta.env.VITE_WS_URL_PREFIX;
  const finalUrl = injected || envUrl;
  if (!finalUrl) {
    console.warn('[wsStore] Missing VITE_WS_URL_PREFIX and no window.__APP_CFG.wsUrl override provided');
  }
  return finalUrl;
}

export const useWsStore = defineStore('ws', {
  state: () => ({
    wsUrl: resolveWsUrl(),
  })
})