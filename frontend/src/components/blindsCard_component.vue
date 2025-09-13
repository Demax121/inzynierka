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
                
                <div class="lux-config-form">
                    <div class="input-group">
                        <label for="minLux">Min lux:</label>
                        <input 
                            id="minLux"
                            v-model.number="minLux" 
                            type="number" 
                            min="0" 
                            placeholder="Minimalna wartość lux"
                            class="lux-input"
                        />
                    </div>
                    <div class="input-group">
                        <label for="maxLux">Max lux:</label>
                        <input 
                            id="maxLux"
                            v-model.number="maxLux" 
                            type="number" 
                            min="0" 
                            placeholder="Maksymalna wartość lux"
                            class="lux-input"
                        />
                    </div>
                    <button class="btn" @click="saveLuxConfig" :disabled="loading">
                        {{ loading && currentAction === 'save' ? 'Zapisywanie...' : 'Ustaw granice' }}
                    </button>
                </div>

                <div class="button-group">
                    <button class="btn" @click="openBlinds" :disabled="loading">
                        {{ loading && currentAction === 'open' ? 'Otwieranie...' : 'Otwórz rolety' }}
                    </button>

                    <button class="btn" @click="closeBlinds" :disabled="loading">
                        {{ loading && currentAction === 'close' ? 'Zamykanie...' : 'Zamknij rolety' }}
                    </button>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useLinkStore } from '@/stores/linkStore'

// Reactive state
const status = ref('')
const loading = ref(false)
const responseData = ref(null)
const currentAction = ref('')
const minLux = ref(0)
const maxLux = ref(0)

// Initialize the link store
const linkStore = useLinkStore()

// Generic function to make API calls for blinds control
const makeApiCall = async (phpFile, actionParam, action, statusMessage, successMessage) => {
    loading.value = true
    currentAction.value = action
    status.value = statusMessage
    responseData.value = null

    try {
        let apiUrl = linkStore.getPhpApiUrl(phpFile)
        
        // Add action parameter for the unified blinds control
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
    await makeApiCall('blindsControl.php', 'open', 'open', 'Otwieranie rolet...', 'Rolety zostały otwarte!')
}

// Function to close blinds
const closeBlinds = async () => {
    await makeApiCall('blindsControl.php', 'close', 'close', 'Zamykanie rolet...', 'Rolety zostały zamknięte!')
}


const saveLuxConfig = async () => {
    status.value = 'Zapisano'
    if (minLux.value >= maxLux.value) {
        status.value = 'Błąd: Min lux musi być mniejsze niż Max lux.'
        return
    }
  const res = await fetch(linkStore.getPhpApiUrl('saveBlindsConfig.php'), {
    method: 'POST',
    headers: { 'Content-Type':'application/json' },
    body: JSON.stringify({ minLux:minLux.value, maxLux:maxLux.value })
  })
}



</script>

<style lang="scss" scoped>
// SCSS Variables for styling
$input-width: 120px;
$input-padding: 6px 10px;
$input-font-size: 11pt;
$form-margin-bottom: 0rem;
$input-group-gap: 0.4rem;
$button-padding: 11px 15px;
$button-font-size: 12pt;

.lux-config-form {
    margin-bottom: $form-margin-bottom;
    padding: 1rem;
    border: 1px solid #ddd;
    border-radius: 8px;
    background-color: #f9f9f9;
    text-align: center;
}

.input-group {
    display: flex;
    align-items: center;
    margin-bottom: $input-group-gap;
    gap: 1rem;
    justify-content: center;
}

.input-group label {
    min-width: 70px;
    font-weight: bold;
    font-size: 14px;
}

.lux-input {
    padding: $input-padding;
    font-size: $input-font-size;
    width: $input-width;
    border: 2px solid #ccc;
    border-radius: 4px;
    outline: none;
    transition: border-color 0.3s;
}

.lux-input:focus {
    border-color: var(--color, #007bff);
}

.lux-config-form .btn,
.button-group .btn {
    padding: $button-padding;
    font-size: $button-font-size;
}

.button-group {
    margin-top: 0.5rem;
}

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
