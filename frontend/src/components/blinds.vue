<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Sterowanie roletami</h2>
        </div>
        <div class="card__body">
            <div class="card__content">
                <div class="status" v-if="status">
                    <p class="card__text card__text--bold">{{ status }}</p>
                </div>
                <div class="button-group">
                    <button class="btn" @click="openBlinds" :disabled="loading">
                        {{ loading && currentAction === 'open' ? 'Otwieranie...' : 'Otwórz rolety' }}
                    </button>

                    <button class="btn" @click="closeBlinds" :disabled="loading">
                        {{ loading && currentAction === 'close' ? 'Zamykanie...' : 'Zamknij rolety' }}
                    </button>

                    <button class="btn" @click="getBlindsStatus" :disabled="loading">
                        {{ loading && currentAction === 'status' ? 'Pobieranie...' : 'Sprawdź status' }}
                    </button>
                </div>
            </div>
        </div>
        <div class="card__footer" v-if="deviceInfo">
            <div class="card__info-item">
                <span class="card__label">Poziom baterii:</span>
                <span class="card__value">{{ deviceInfo.battery_percentage }}%</span>
            </div>
            <div class="card__info-item">
                <span class="card__label">Status:</span>
                <span class="card__value">{{ controlStatus }}</span>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, computed } from 'vue'
import { useLinkStore } from '@/stores/linkStore'

// Reactive state
const status = ref('')
const loading = ref(false)
const responseData = ref(null)
const currentAction = ref('')
const deviceInfo = ref(null)


// Initialize the link store
const linkStore = useLinkStore()

// Generic function to make API calls
const makeApiCall = async (phpFile, action, statusMessage, successMessage) => {
    loading.value = true
    currentAction.value = action
    status.value = statusMessage
    responseData.value = null

    try {
        const apiUrl = linkStore.getPhpApiUrl(phpFile)

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
            status.value = successMessage
        } else {
            status.value = `Błąd: ${data.msg || 'Nieznany błąd'}`
        }

    } catch (error) {
        console.error(`Error with ${action} action:`, error)
        status.value = `Błąd połączenia: ${error.message}`
        responseData.value = { error: error.message }
    } finally {
        loading.value = false
        currentAction.value = ''
    }
}

// Function to open blinds
const openBlinds = async () => {
    await makeApiCall('openBlinds.php', 'open', 'Otwieranie rolet...', 'Rolety zostały otwarte!')
}

// Function to close blinds
const closeBlinds = async () => {
    await makeApiCall('closeBlinds.php', 'close', 'Zamykanie rolet...', 'Rolety zostały zamknięte!')
}

const getBlindsStatus = async () => {
    await makeApiCall('blindsStatus.php', 'status', 'Pobieranie statusu...', 'Status rolet został pobrany!')

    // Extract battery_percentage and control from responseData
    if (responseData.value && responseData.value.result) {
        const result = responseData.value.result;
        const batteryObj = result.find(item => item.code === 'battery_percentage');
        const controlObj = result.find(item => item.code === 'control');
        const info = {
            battery_percentage: batteryObj ? batteryObj.value : null,
            control: controlObj ? controlObj.value : null
        };
        deviceInfo.value = info;
    }
}

const controlStatus = computed(() => {
    if (!deviceInfo.value || deviceInfo.value.control == null) return ''
    if (deviceInfo.value.control === 'close') return 'zasłonięte'
    if (deviceInfo.value.control === 'open') return 'odsłonięte'
    return deviceInfo.value.control
})


</script>

<style lang="scss" scoped>
.device-info {
    border-radius: 10px;
    filter: drop-shadow(0 5px 10px 0 #ffffff);
    width: 15rem;
    height: 5rem;
    background-color: $background-crl-secondary;
    padding: 1rem;
    color: white;
    font-family: "Poppins", sans-serif;
}
</style>