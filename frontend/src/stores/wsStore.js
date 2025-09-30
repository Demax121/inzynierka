// Plik: frontend/src/stores/wsStore.js
import { defineStore } from 'pinia'

export const useWsStore = defineStore('ws', {
  state: () => ({
    // Używamy zmiennej z pliku .env.development
    wsUrl: import.meta.env.VITE_WS_URL_PREFIX
  })
})