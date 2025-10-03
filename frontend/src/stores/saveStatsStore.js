import { defineStore } from 'pinia'
import { useLinkStore } from '@/stores/linkStore'

// Backend PHP endpoint base is resolved dynamically to ensure correct path
// (previously code posted to bare prefix causing CORS/root issues)
const SAVE_INTERVAL = 60 * 60 * 1000
const LAST_SAVED_KEY = 'roomStats_lastSaved'

export const useSaveStatsStore = defineStore('saveStats', {
  state: () => ({
    latest: null,
    intervalId: null,
    startupTimeoutId: null
  }),
  actions: {
    // ustaw dane (np. z komponentu) i uruchom wysyłkę co godzinę
    setStats(stats) {
      this.latest = {
        temperature: Number(stats.temperature),
        humidity: Number(stats.humidity),
        pressure: Number(stats.pressure)
      }
      // jeśli nie mamy zaplanowanych timerów -> uruchom planowanie
      if (!this.intervalId && !this.startupTimeoutId) this.startHourlySave()
    },
    startHourlySave() {
      // nie wykonujemy natychmiastowego zapisu przy odświeżeniu/ponownym wejściu
      const lastSaved = Number(localStorage.getItem(LAST_SAVED_KEY) || 0)
      const now = Date.now()
      let delay = SAVE_INTERVAL

      if (lastSaved && now - lastSaved < SAVE_INTERVAL) {
        // zaplanuj pierwszy zapis po pozostałym czasie do pełnej godziny
        delay = SAVE_INTERVAL - (now - lastSaved)
      } else {
        // brak zapisu w ciągu ostatniej godziny -> pierwsze zapisanie za pełną godzinę
        delay = SAVE_INTERVAL
      }

      // ustaw timeout, po którym wykonujemy pierwszy zapis i potem ustalamy interval
      this.startupTimeoutId = setTimeout(async () => {
        this.startupTimeoutId = null
        await this.saveToServer()
        // zabezpieczamy przed wielokrotnym tworzeniem intervalu
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
    // opcjonalne wymuszenie zapisu natychmiast
    async forceSaveNow() {
      await this.saveToServer(true)
    },
    async saveToServer(forced = false) {
      if (!this.latest) return
      // jeśli nie wymuszone i ostatni zapis był < 1h temu, pomiń
      const lastSaved = Number(localStorage.getItem(LAST_SAVED_KEY) || 0)
      if (!forced && lastSaved && Date.now() - lastSaved < SAVE_INTERVAL) return

      try {
        const linkStore = useLinkStore()
        const endpoint = linkStore.getPhpApiUrl('saveRoomStats.php')
        const res = await fetch(endpoint, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            temperature: this.latest.temperature,
            humidity: this.latest.humidity,
            pressure: this.latest.pressure
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