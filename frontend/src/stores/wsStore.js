import { defineStore } from 'pinia'

export const useWsStore = defineStore('ws', {
  state: () => ({
    wsUrl: 'ws://192.168.1.2:8886'
  })
})