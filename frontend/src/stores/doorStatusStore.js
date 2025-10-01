
import { defineStore } from 'pinia'
import { useLinkStore } from './linkStore'

const LOCAL_KEY = 'door_last_status'

export const useDoorStatusStore = defineStore('doorStatus', {
  state: () => ({
    last: localStorage.getItem(LOCAL_KEY) || null
  }),
  actions: {
    async saveDoorStatus(status) {
      if (this.last === status) return false

      // Get the PHP API URL for doorStatus.php from linkStore
      const linkStore = useLinkStore()
      const apiUrl = linkStore.getPhpApiUrl('doorStatus.php')

      try {
        const res = await fetch(apiUrl, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ status })
        })

        if (!res.ok) throw new Error(res.status)

        // aktualizacja tylko po sukcesie
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
