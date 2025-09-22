<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Sterowanie roletami</h2>
        </div>
        <div class="card__body">
            <div class="card__content">

                <div class="button-group">
                    <button type="button" class="btn" @click="openBlinds">
                        Otwórz rolety
                    </button>

                    <button type="button" class="btn" @click="closeBlinds">
                        Zamknij rolety
                    </button>
                    <button type="button" class="btn" @click="fetchStatus()">
                        Pobierz status
                    </button>
                </div>
                <div class="device-info" v-if="batteryLevel !== null || deviceState !== null">
                    <p class="card__text"><b>Poziom baterii: </b> {{ batteryLevel !== null ? batteryLevel + '%' : 'Brak danych' }}</p>
                    <p class="card__text"><b>Stan rolet: </b> {{ deviceStateText }}</p>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, computed } from 'vue'
import { useLinkStore } from '@/stores/linkStore'
import { useAutomateStore } from '@/stores/automateStore'

const responseData = ref(null)
const batteryLevel = ref(null)
const deviceState = ref(null) 

const linkStore = useLinkStore()
const automateStore = useAutomateStore()

const setAutomateFalse = async () => {
    automateStore.automate_flag = false
    try {
        await fetch(linkStore.getPhpApiUrl('setBlindsAutomate.php'), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ automate: false })
        })
    } catch (e) {
        // Silent error
    }
}

const makeApiCall = async (phpFile, actionParam) => {
    try {
        let apiUrl = linkStore.getPhpApiUrl(phpFile)
        
        if (actionParam) {
            apiUrl += `?action=${actionParam}`
        }

        const response = await fetch(apiUrl, {
            method: 'GET',
            headers: {
                'Content-Type': 'application/json',
            },
        })

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`)
        }

        const data = await response.json()
        responseData.value = data

        if (data.success) {
            fetchStatus().catch(() => {})
        }
    } catch (error) {
        // API call error handled silently
    }
}

const openBlinds = async () => {
    await setAutomateFalse()
    await makeApiCall('blindsControl.php', 'open')
    setTimeout(fetchStatus, 5000);
}

const closeBlinds = async () => {
    await setAutomateFalse()
    await makeApiCall('blindsControl.php', 'close')
    setTimeout(fetchStatus, 5000);
}

const parseStatus = (data) => {
    
    batteryLevel.value = null
    deviceState.value = null

    if (!data) return

    const result = data.result ?? data

    let items = []
    if (Array.isArray(result)) {
        items = result
    } else if (result && typeof result === 'object') {

        items = result.list ?? result.status ?? []
        if (!Array.isArray(items)) items = []
    }

    const numericStateCodes = ['control', 'state', 'switch', 'status']
    const textStateCodes = ['control', 'state', 'switch', 'status']

    for (const it of items) {
        const code = (it.code || '').toLowerCase()
        const value = it.value

        if (code.includes('battery') && (typeof value === 'number' || typeof value === 'string')) {
            const num = Number(value)
            if (!Number.isNaN(num)) batteryLevel.value = num
        }

        if (typeof value === 'string' && (textStateCodes.includes(code) || code.includes('control'))) {
            const v = value.toLowerCase()
            if (v.includes('open')) deviceState.value = 'open'
            if (v.includes('close')) deviceState.value = 'close'
            continue
        }

        if (typeof value === 'number' && numericStateCodes.includes(code)) {
            if (value === 1) deviceState.value = 'open'
            else if (value === 0) deviceState.value = 'close'
        } else if (typeof value === 'boolean' && (textStateCodes.includes(code) || code.includes('control'))) {
            deviceState.value = value ? 'open' : 'close'
        }
    }
}

const fetchStatus = async () => {
    try {
        const apiUrl = linkStore.getPhpApiUrl('getBlindsStatus.php')
        const resp = await fetch(apiUrl, { method: 'GET' })
        if (!resp.ok) throw new Error(`HTTP ${resp.status}`)
        const data = await resp.json()
        responseData.value = data
        
        // Handle the new response format directly
        if (data) {
            // Set battery level directly from response
            if (data.battery_percent !== undefined && data.battery_percent !== null) {
                batteryLevel.value = Number(data.battery_percent)
            }
            
            // Set device state based on blinds_state value
            if (data.blinds_state !== undefined) {
                const state = String(data.blinds_state).toLowerCase()
                if (state === 'open' || state === '1' || state === 'true') {
                    deviceState.value = 'open'
                } else if (state === 'close' || state === '0' || state === 'false') {
                    deviceState.value = 'close'
                }
            } else {
                // If no state in response, try parsing the whole response with the old method
                parseStatus(data)
            }
        }
    } catch (err) {
        // fetchStatus error handled silently
    }
}

const deviceStateText = computed(() => {
    if (deviceState.value === 'open') return 'Otwarte'
    if (deviceState.value === 'close') return 'Zamknięte'
    return 'Nieznany'
})

onMounted(() => {
    fetchStatus().catch(() => { })
})
</script>

<style lang="scss" scoped>
$button-padding: 11px 15px;
$button-font-size: 12pt;

.button-group {
    margin-top: 0.5rem;
}

.device-info {
    filter: drop-shadow(0 5px 10px 0 #ffffff);
    width: 15rem;
    height: 5rem;
    padding: 1rem;
    color: white;
    font-size: 13pt;
}

.card__text {
    font-size: inherit;
}
</style>