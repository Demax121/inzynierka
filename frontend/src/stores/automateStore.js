import { defineStore } from 'pinia'
import { ref } from 'vue'

export const useAutomateStore = defineStore('automate', () => {
  const automate_flag = ref(false)
  return { automate_flag }
})