<script setup>
import { ref, reactive, onMounted, onUnmounted } from 'vue'
import { useLinkStore } from '@/stores/linkStore'
import { useAutomateStore } from '@/stores/automateStore'
import { useWsStore } from '@/stores/wsStore'; // Importujemy wsStore

const status = ref('')
const loading = ref(false)
const lastAutomateChange = ref(Date.now())
const AUTOMATE_DEBOUNCE = 2000
let ws

const linkStore = useLinkStore()
const automateStore = useAutomateStore()
const wsStore = useWsStore(); // Inicjalizujemy wsStore

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
  ws = new WebSocket(wsStore.wsUrl) // Używamy adresu ze store'a
  
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