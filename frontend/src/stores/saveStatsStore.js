import { defineStore } from 'pinia'
import { useLinkStore } from './linkStore'

const SAVE_INTERVAL = 60 * 60 * 1000
const LAST_SAVED_KEY = 'roomStats_lastSaved'

export const useSaveStatsStore = defineStore('saveStats', {
  state: () => ({
    latest: null,
    intervalId: null,
    startupTimeoutId: null
  }),
  actions: {
    setStats(stats) {
      this.latest = {
        temperature: parseFloat(stats.temperature),
        humidity: parseFloat(stats.humidity),
        pressure: parseFloat(stats.pressure)
      }
      if (!this.intervalId && !this.startupTimeoutId) this.startHourlySave()
    },
    startHourlySave() {
      const lastSaved = Number(localStorage.getItem(LAST_SAVED_KEY) || 0)
      const now = Date.now()
      let delay = SAVE_INTERVAL

      if (lastSaved && now - lastSaved < SAVE_INTERVAL) {
        delay = SAVE_INTERVAL - (now - lastSaved)
      } else {
        delay = SAVE_INTERVAL
      }

      this.startupTimeoutId = setTimeout(async () => {
        this.startupTimeoutId = null
        await this.saveToServer()
        if (this.intervalId) return
        this.intervalId = setInterval(() => this.saveToServer(), SAVE_INTERVAL)
      }, delay)
    },
    stopHourlySave() {
      if (this.startupTimeoutId) {
        clearTimeout(this.startupTimeoutId)
        this.startupTimeoutId = null
      }
      if (this.intervalId) {
        clearInterval(this.intervalId)
        this.intervalId = null
      }
    },
    async forceSaveNow() {
      await this.saveToServer(true)
    },
    async saveToServer(forced = false) {
      if (!this.latest) return
      const lastSaved = Number(localStorage.getItem(LAST_SAVED_KEY) || 0)
      if (!forced && lastSaved && Date.now() - lastSaved < SAVE_INTERVAL) return

      const linkStore = useLinkStore()
      try {
        const res = await fetch(linkStore.getPhpApiUrl('saveRoomStats.php'), {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            temperature: parseFloat(this.latest.temperature),
            humidity: parseFloat(this.latest.humidity),
            pressure: parseFloat(this.latest.pressure)
          })
        })
        if (res.ok) {
          localStorage.setItem(LAST_SAVED_KEY, Date.now().toString())
        } else {
          console.error('saveToServer response not ok', res.status)
        }
      } catch (e) {
        console.error('saveToServer error', e)
      }
    }
  }
})