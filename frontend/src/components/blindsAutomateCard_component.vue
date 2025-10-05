<template>
  <!-- Card shell (global .card styles from containers.scss) -->
  <div class="card card--blinds-auto">
    <div class="card__header">
      <h2 class="card__title">Blinds Automation</h2>
    </div>

    <div class="card__body">
      <!-- BEM block: blinds-auto -->
      <div class="card__content blinds-auto">

        <!-- Status / feedback message (auto‑clearing) -->
        <div
          v-if="status"
          class="blinds-auto__status"
          role="status"
          aria-live="polite"
        >
          <p class="card__text card__text--bold blinds-auto__status-message">
            {{ status }}
          </p>
        </div>

        <!-- Configuration form (lux thresholds + automate toggle) -->
        <form
          class="blinds-auto__form"
          @submit.prevent="saveConfig()"
        >
          <!-- Min lux -->
            <div class="blinds-auto__group">
              <label
                class="blinds-auto__label"
                for="min_lux"
              >Min lux:</label>
              <input
                id="min_lux"
                v-model.number="config.min_lux"
                type="number"
                class="blinds-auto__input"
                inputmode="numeric"
                :aria-invalid="validateLuxPair(config.min_lux, config.max_lux) ? 'true' : 'false'"
              />
            </div>

          <!-- Max lux -->
            <div class="blinds-auto__group">
              <label
                class="blinds-auto__label"
                for="max_lux"
              >Max lux:</label>
              <input
                id="max_lux"
                v-model.number="config.max_lux"
                type="number"
                class="blinds-auto__input"
                inputmode="numeric"
                :aria-invalid="validateLuxPair(config.min_lux, config.max_lux) ? 'true' : 'false'"
              />
            </div>

          <!-- Automate toggle (modifier group) -->
            <div class="blinds-auto__group blinds-auto__group--toggle">
              <span class="blinds-auto__toggle-label">Automatic mode:</span>
              <label class="switch switch--small blinds-auto__switch">
                <input
                  type="checkbox"
                  v-model="config.automate"
                  @change="toggleAutomate"
                  aria-label="Toggle blinds automation"
                >
                <span class="slider"></span>
              </label>
            </div>

          <!-- Actions (submit button) -->
          <div class="blinds-auto__actions">
            <button
              type="submit"
              class="btn blinds-auto__btn"
              :disabled="loading"
            >
              {{ loading ? 'Saving...' : 'Set limits' }}
            </button>
          </div>
        </form>

      </div>
    </div>
  </div>
</template>

<script setup>
/*
  Blinds Automation Card (BEM refactor: block = blinds-auto)
  Responsibilities:
    - Load existing blinds automation config (min_lux, max_lux, automate flag)
    - Allow user to adjust thresholds and toggle automation
    - Persist changes (saveBlindsConfig.php)
    - React to live lux_sensor WebSocket data and issue Tuya open/close commands
      based on thresholds when automation is enabled
    - Debounce rapid automation decisions (AUTOMATE_DEBOUNCE)
*/

import { ref, reactive, onMounted, onUnmounted } from 'vue'
import { useWsStore } from '@/stores/wsStore'
import { useLinkStore } from '@/stores/linkStore'
import { useAutomateStore } from '@/stores/automateStore'

/* Reactive UI state */
const status = ref('')                       // UX feedback message
const loading = ref(false)                   // Network action state
const lastAutomateChange = ref(Date.now())   // Timestamp of last auto decision
const AUTOMATE_DEBOUNCE = 2000               // ms debounce to avoid rapid toggles
let ws                                        // WebSocket reference
const wsStore = useWsStore()

/* Stores */
const linkStore = useLinkStore()
const automateStore = useAutomateStore()

/* Validation boundaries (mirror backend) */
const LUX_MIN = 0
const LUX_MAX = 1000000

/* Config object (reactive). automate proxied via automateStore */
const config = reactive({
  min_lux: 0,
  max_lux: 0,
  get automate () { return automateStore.automate_flag },
  set automate (val) { automateStore.automate_flag = val }
})

/* Validation helper for paired lux values */
function validateLuxPair (minVal, maxVal) {
  if (!Number.isFinite(minVal) || !Number.isFinite(maxVal)) {
    return 'Lux values must be numeric'
  }
  if (minVal < LUX_MIN || maxVal < LUX_MIN || minVal > LUX_MAX || maxVal > LUX_MAX) {
    return `Lux values out of range (${LUX_MIN}-${LUX_MAX})`
  }
  if (minVal >= maxVal) {
    return 'Min lux must be less than Max lux'
  }
  return null
}

/* Fetch current config from backend */
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

/* Status helper (auto-clear) */
const setStatus = (message, timeout = 3000) => {
  status.value = message
  setTimeout(() => { status.value = '' }, timeout)
}

/* Toggle automation and persist current thresholds */
const toggleAutomate = () => {
  loading.value = true
  status.value = 'Saving...'

  saveConfig(true) // quiet mode (we manage status manually)
    .then(() => {
      setStatus(config.automate ? 'Automation enabled' : 'Automation disabled')
      lastAutomateChange.value = Date.now()
    })
    .catch((error) => {
      // Revert on failure
      config.automate = !config.automate
      setStatus(`Error: ${error.message}`)
    })
    .finally(() => { loading.value = false })
}

/* Fire Tuya command (open | close); silent errors by design */
const controlBlinds = async (action) => {
  try {
    await fetch(linkStore.getPhpApiUrl('tuyaBlindsApi.php') + `?action=${action}`)
  } catch {
    /* swallow */
  }
}

/* Persist thresholds + automate flag */
const saveConfig = async (quiet = false) => {
  if (!quiet) {
    loading.value = true
    status.value = 'Saving...'
  }

  const err = validateLuxPair(Number(config.min_lux), Number(config.max_lux))
  if (err && !quiet) {
    status.value = 'Error: ' + err
    loading.value = false
    return Promise.reject(new Error(err))
  }

  try {
    const res = await fetch(linkStore.getPhpApiUrl('saveBlindsConfig.php'), {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        min_lux: Number(config.min_lux),
        max_lux: Number(config.max_lux),
        automate: automateStore.automate_flag
      })
    })
    const data = await res.json()

    if (data?.error) {
      if (!quiet) setStatus(`Error while saving: ${data.error}`)
      throw new Error(data.error)
    }

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

/* Decide open/close based on live lux data */
const handleLuxAutomation = (luxValue) => {
  const isAutomated = config.automate
  if (
    !isAutomated ||
    Date.now() - lastAutomateChange.value <= AUTOMATE_DEBOUNCE ||
    luxValue === null
  ) return

  if (luxValue < config.min_lux || luxValue >= config.max_lux) {
    controlBlinds('close')
  } else {
    controlBlinds('open')
  }
}

/* Lifecycle: mount (fetch config + WS subscribe) */
onMounted(() => {
  getConfig()

  ws = new WebSocket(wsStore.wsUrl)
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data)
      if (data.channel === 'lux_sensor' && data.lux !== undefined) {
        handleLuxAutomation(data.lux)
      }
    } catch {
      /* ignore malformed frames */
    }
  }
})

/* Cleanup */
onUnmounted(() => {
  if (ws) ws.close()
})
</script>

<style lang="scss" scoped>
/*
  BEM: .blinds-auto (local block)
  Keep global .card untouched (from containers.scss).
  All previous ad-hoc classes replaced with BEM elements/modifiers.
*/

$ba-border: 1px solid #ddd;
$ba-bg: #f9f9f9;
$ba-radius: 0.5rem;
$ba-gap: 0.75rem;
$ba-gap-sm: 0.4rem;
$ba-accent: var(--color, #007bff);
$ba-label-fz: 0.875rem;
$ba-input-width: 7.5rem;
$ba-transition: 0.3s;

.card--blinds-auto {
  /* reserved for future card-level overrides */
}

.blinds-auto {
  display: flex;
  flex-direction: column;
  gap: $ba-gap;

  &__status {
    text-align: center;
    padding: 0.25rem 0.5rem;
    animation: fade-in 0.25s ease;
  }
  &__status-message {
    margin: 0;
  }

  &__form {
    margin: 0;
    padding: 1rem;
    border: $ba-border;
    border-radius: $ba-radius;
    background: $ba-bg;
    display: flex;
    flex-direction: column;
    gap: $ba-gap;
  }

  &__group {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 1rem;

    &--toggle {
      margin: 0.75rem 0 0.25rem;
    }
  }

  &__label,
  &__toggle-label {
    min-width: 4.5rem;
    font-weight: 600;
    font-size: $ba-label-fz;
    text-align: right;
  }

  &__toggle-label {
    text-align: left;
  }

  &__input {
    width: $ba-input-width;
    padding: 0.4rem 0.625rem;
    font-size: 0.95rem;
    line-height: 1.2;
    border: 2px solid #ccc;
    border-radius: 0.25rem;
    outline: none;
    transition: border-color $ba-transition;

    &:focus {
      border-color: $ba-accent;
    }
    &[aria-invalid='true'] {
      border-color: #d33;
    }
  }

  &__switch {
    display: inline-flex;
  }

  &__actions {
    display: flex;
    justify-content: center;
    margin-top: 0.25rem;
  }

  &__btn {
    min-width: 8rem;
  }
}

/* Simple fade for status transitions */
@keyframes fade-in {
  from { opacity: 0; transform: translateY(-2px); }
  to   { opacity: 1; transform: translateY(0); }
}
</style>