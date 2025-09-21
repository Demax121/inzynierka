<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title">Automatyka rolet</h2>
    </div>
    <div class="card__body">
      <div class="card__content">
        <div class="status" v-if="status">
          <p class="card__text card__text--bold">{{ status }}</p>
        </div>

        <div class="lux-config-form">
          <div class="input-group">
            <label for="minLux">Min lux:</label>
            <input id="minLux" v-model.number="config.minLux" type="number" class="lux-input" />
          </div>
          <div class="input-group">
            <label for="maxLux">Max lux:</label>
            <input id="maxLux" v-model.number="config.maxLux" type="number" class="lux-input" />
          </div>

          <div class="input-group input-group--slider">
            <span class="slider-label">Tryb automatyczny:</span>
            <label class="switch switch--small">
              <input type="checkbox" v-model="config.automate" @change="toggleAutomate">
              <span class="slider"></span>
            </label>
          </div>
        </div>

        <div class="button-group">
          <button class="btn" @click="saveConfig" :disabled="loading">
            {{ loading ? 'Zapisywanie...' : 'Ustaw granice' }}
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, onUnmounted } from 'vue'
import { useLinkStore } from '@/stores/linkStore'

const status = ref('')
const loading = ref(false)
const lastAutomateChange = ref(Date.now())
const AUTOMATE_DEBOUNCE = 2000
let ws

// Consolidated state
const config = reactive({
  minLux: 0,
  maxLux: 0,
  automate: false
})

const linkStore = useLinkStore()

// Fetch configuration from the server
const getConfig = async () => {
  loading.value = true
  status.value = 'Ładowanie konfiguracji...'
  try {
    const res = await fetch(linkStore.getPhpApiUrl('getBlindsConfig.php'))
    const data = await res.json()

    if (data && typeof data === 'object') {
      config.minLux = Number(data.min_lux ?? 0)
      config.maxLux = Number(data.max_lux ?? 0)
      // This is the only place where automate should be updated from server data
      config.automate = Boolean(data.automate ?? false)
      status.value = 'Konfiguracja załadowana.'
      setTimeout(() => { status.value = '' }, 3000)
    } else {
      setStatus('Błąd: Nieprawidłowy format odpowiedzi.')
    }
  } catch (error) {
    setStatus(`Błąd połączenia: ${error.message}`)
  } finally {
    loading.value = false
  }
}

// Set status with auto-clear after delay
const setStatus = (message, timeout = 3000) => {
  status.value = message
  setTimeout(() => { status.value = '' }, timeout)
}

// Toggle automation mode and save configuration
const toggleAutomate = () => {
  // Save configuration to database
  loading.value = true
  status.value = 'Zapisywanie...'
  
  saveConfig(true) // Use quiet mode to handle our own loading state
    .then((res) => {
      // After save completes, show automation-specific message
      setStatus(config.automate ? 'Automatyka włączona' : 'Automatyka wyłączona')
      lastAutomateChange.value = Date.now()
    })
    .catch((error) => {
      // If saving fails, revert the toggle and show error
      config.automate = !config.automate // Revert the toggle
      setStatus(`Błąd: ${error.message}`)
    })
    .finally(() => {
      loading.value = false
    })
}

// Control blinds
const controlBlinds = async (action) => {
  try {
    await fetch(linkStore.getPhpApiUrl('blindsControl.php') + `?action=${action}`)
  } catch (error) {
    // Silent error handling
  }
}

// Save configuration to server - update to return a promise for chaining
const saveConfig = async (quiet = false) => {
  if (!quiet) {
    loading.value = true
    status.value = 'Zapisywanie...'
  }

  if (config.minLux >= config.maxLux && !quiet) {
    status.value = 'Błąd: Min lux musi być mniejsze niż Max lux.'
    loading.value = false
    return Promise.reject(new Error('Min lux musi być mniejsze niż Max lux'));
  }

  try {
    const res = await fetch(linkStore.getPhpApiUrl('saveBlindsConfig.php'), {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        minLux: config.minLux,
        maxLux: config.maxLux,
        automate: config.automate
      })
    });

    const data = await res.json();
    
    if (!quiet) {
      setStatus(data?.success ? 'Zapisano pomyślnie' : 'Błąd podczas zapisywania')
    }
    
    return data; // Return the data for promise chaining
  } catch (error) {
    if (!quiet) {
      setStatus(`Błąd połączenia: ${error.message}`)
    }
    throw error; // Re-throw to allow error handling in the promise chain
  } finally {
    if (!quiet) {
      loading.value = false
    }
  }
}

// Handle automation based on lux readings
const handleLuxAutomation = (luxValue) => {
  // Store automation state in a local constant to avoid any inadvertent changes
  const isAutomated = config.automate
  
  if (!isAutomated || 
      Date.now() - lastAutomateChange.value <= AUTOMATE_DEBOUNCE ||
      luxValue === null) {
    return
  }
  
  if (luxValue < config.minLux || luxValue >= config.maxLux) {
    // Too dark or too bright, close blinds
    controlBlinds('close')
  } else {
    // Ideal range, open blinds
    controlBlinds('open')
  }
}

onMounted(() => {
  getConfig() // Initial load of config from server
  
  // Setup WebSocket
  ws = new WebSocket('ws://192.168.1.4:8886')
  
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data)
      if (data.channel === 'lux_sensor' && data.lux !== undefined) {
        // Just pass the lux value - don't modify any other state
        handleLuxAutomation(data.lux)
      }
    } catch (error) {
      // Silent error handling
    }
  }
})

onUnmounted(() => {
  if (ws) ws.close()
})
</script>

<style lang="scss" scoped>
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
  
  &--slider {
    margin: 1rem 0;
  }
}

.slider-label {
  min-width: 70px;
  font-weight: bold;
  font-size: 14px;
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
