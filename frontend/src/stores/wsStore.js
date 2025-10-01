import { defineStore } from 'pinia'

export const useWsStore = defineStore('useWsStore', {
  state: () => ({
    wsUrl: import.meta.env.VITE_WS_URL_PREFIX
  })
})