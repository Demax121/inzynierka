import { defineStore } from 'pinia'
import { useLinkStore } from '@/stores/linkStore'

// Local storage key for caching last known door status
const LOCAL_KEY = 'door_last_status'
// Default debounce delay (ms) for saving door status to backend
const DEFAULT_DEBOUNCE = 5000

export const useDoorStatusStore = defineStore('doorStatus', {
  state: () => ({
    last: localStorage.getItem(LOCAL_KEY) || null,
    _pendingStatus: null,          // status waiting to be persisted
    _debounceTimer: null,          // timer id for debounce
    _debounceDelay: DEFAULT_DEBOUNCE
  }),
  actions: {
    /**
     * Configure debounce delay at runtime (e.g. from a settings panel)
     * @param {number} ms milliseconds (minimum 0)
     */
    setDebounceDelay(ms) {
      const val = Number(ms)
      if (!isNaN(val) && val >= 0) {
        this._debounceDelay = val
        // If delay set to 0 and there's a pending status -> flush immediately
        if (val === 0 && this._pendingStatus) this.flushNow()
      }
    },
    /**
     * Enqueue door status for saving (debounced). Multiple rapid changes collapse to last.
     * @param {string} status 'Door open' | 'Door closed'
     */
    enqueueStatus(status) {
      if (status !== 'Door open' && status !== 'Door closed') return
      // If already persisted to this value and nothing pending -> ignore
      if (!this._pendingStatus && this.last === status) return
      this._pendingStatus = status
      if (this._debounceDelay === 0) {
        this.flushNow()
        return
      }
      if (this._debounceTimer) clearTimeout(this._debounceTimer)
      this._debounceTimer = setTimeout(() => this.flushNow(), this._debounceDelay)
    },
    /**
     * Force immediate persistence of pending status (if any)
     */
    async flushNow() {
      if (!this._pendingStatus) return false
      const target = this._pendingStatus
      this._pendingStatus = null
      if (this._debounceTimer) {
        clearTimeout(this._debounceTimer)
        this._debounceTimer = null
      }
      return await this.saveDoorStatus(target)
    },
    async saveDoorStatus(status) {
      if (this.last === status) return false // avoid redundant writes

      const linkStore = useLinkStore()
      const endpoint = linkStore.getPhpApiUrl('saveDoorStatus.php')

      try {
        const res = await fetch(endpoint, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ status })
        })

        if (!res.ok) {
          let details = ''
            try { details = await res.text() } catch {}
          throw new Error(`HTTP ${res.status} ${details}`)
        }

        this.last = status
        localStorage.setItem(LOCAL_KEY, status)
        return true
      } catch (e) {
        console.error('doorStatus save error', e)
        return false
      }
    }
  }
})
