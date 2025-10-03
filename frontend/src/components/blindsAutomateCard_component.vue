<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title">Blinds Automation</h2>
    </div>
    <div class="card__body">
      <div class="card__content">
        <div class="status" v-if="status">
          <p class="card__text card__text--bold">{{ status }}</p>
        </div>

        <div class="lux-config-form">
          <div class="input-group">
            <label for="min_lux">Min lux:</label>
            <input id="min_lux" v-model.number="config.min_lux" type="number" class="lux-input" />
          </div>
          <div class="input-group">
            <label for="max_lux">Max lux:</label>
            <input id="max_lux" v-model.number="config.max_lux" type="number" class="lux-input" />
          </div>

          <div class="input-group input-group--slider">
            <span class="slider-label">Automatic mode:</span>
            <label class="switch switch--small">
              <input type="checkbox" v-model="config.automate" @change="toggleAutomate">
              <span class="slider"></span>
            </label>
          </div>
        </div>

        <div class="button-group">
          <button class="btn" @click="saveConfig" :disabled="loading">
            {{ loading ? 'Saving...' : 'Set limits' }}
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, onUnmounted } from 'vue'
import { useLinkStore } from '@/stores/linkStore'
import { useAutomateStore } from '@/stores/automateStore'

const status = ref('')
const loading = ref(false)
const lastAutomateChange = ref(Date.now())
const AUTOMATE_DEBOUNCE = 2000
let ws

const linkStore = useLinkStore()
const automateStore = useAutomateStore()

// Consolidated state
const config = reactive({
  min_lux: 0,
  max_lux: 0,
  get automate() { return automateStore.automate_flag },
  set automate(val) { automateStore.automate_flag = val }
})

const getConfig = async () => {
  loading.value = true
  status.value = 'Loading configuration...'
  try {
    const res = await fetch(linkStore.getPhpApiUrl('getBlindsConfig.php'))
    const data = await res.json()

    if (data?.error) {
      setStatus(`Error: ${data.error}`)
      return
    }

    config.min_lux = Number(data.min_lux ?? 0)
    config.max_lux = Number(data.max_lux ?? 0)
    automateStore.automate_flag = Boolean(data.automate ?? false)

    status.value = 'Configuration loaded.'
    setTimeout(() => { status.value = '' }, 3000)
  } catch (error) {
    setStatus(`Connection error: ${error.message}`)
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
  status.value = 'Saving...'
  
  saveConfig(true) // Use quiet mode to handle our own loading state
    .then((res) => {
      // After save completes, show automation-specific message
      setStatus(config.automate ? 'Automation enabled' : 'Automation disabled')
      lastAutomateChange.value = Date.now()
    })
    .catch((error) => {
      // If saving fails, revert the toggle and show error
      config.automate = !config.automate // Revert the toggle
      setStatus(`Error: ${error.message}`)
    })
    .finally(() => {
      loading.value = false
    })
}

// Control blinds
const controlBlinds = async (action) => {
  try {
    await fetch(linkStore.getPhpApiUrl('tuyaBlindsApi.php') + `?action=${action}`)
  } catch (error) {
    // Silent error handling
  }
}

const saveConfig = async (quiet = false) => {
  if (!quiet) {
    loading.value = true
    status.value = 'Saving...'
  }

  if (config.min_lux >= config.max_lux && !quiet) {
    status.value = 'Error: Min lux must be less than Max lux.'
    loading.value = false
    return Promise.reject(new Error('Min lux must be less than Max lux'))
  }

  try {
    const res = await fetch(linkStore.getPhpApiUrl('saveBlindsConfig.php'), {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        min_lux: config.min_lux,
        max_lux: config.max_lux,
        automate: automateStore.automate_flag
      })
    })

    const data = await res.json()

    if (data?.error) {
      if (!quiet) setStatus(`Error while saving: ${data.error}`)
      throw new Error(data.error)
    }

    // update config after save
    config.min_lux = Number(data.min_lux)
    config.max_lux = Number(data.max_lux)
    config.automate = Boolean(data.automate)

    if (!quiet) setStatus('Saved successfully')

    return data
  } catch (error) {
    if (!quiet) setStatus(`Connection error: ${error.message}`)
    throw error
  } finally {
    if (!quiet) loading.value = false
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
  
  if (luxValue < config.min_lux || luxValue >= config.max_lux) {
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
  ws = new WebSocket(import.meta.env.VITE_WS_URL_PREFIX)
  
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