import { defineStore } from 'pinia'

export const useWsStore = defineStore('ws', {
  state: () => ({
    wsUrl: import.meta.env.VITE_WS_URL
  })
})