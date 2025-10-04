<template>
  <!--
    Block wrapper: card + modifier card--ac for future AC‑specific styling.
    Inner functional block .ac holds the AC controls & readings.
  -->
  <div class="card card--ac">
    <div class="card__header">
      <h2 class="card__title card__title--ac">Air Conditioning</h2>
    </div>

    <div class="card__body">
      <div class="ac">
        <!-- Loading state placeholder (replaces controls until first payload arrives) -->
        <LoadingCard v-if="loading" class="ac__loading" />

        <!-- Main content shown once we have AC payload -->
        <div
          v-else
          class="card__content card__content--slider ac__content"
        >
          <!-- Temperature adjuster: two buttons + live editable local temp (inputTemp) -->
            <div class="ac__temp-form">
              <button
                type="button"
                class="btn btn--square ac__btn"
                @click="changeTemp(-0.5)"
              >-0.5</button>

              <!-- Display targeted temp (local state pending server confirmation) -->
              <span class="ac__temp-display">{{ inputTemp }}°C</span>

              <button
                type="button"
                class="btn btn--square ac__btn"
                @click="changeTemp(0.5)"
              >+0.5</button>
            </div>

            <!-- Readouts: current sensor temp, target temp, mode and manual override flag -->
            <div class="ac__temps">
              <p class="ac__temps-line">
                Current:
                <strong class="ac__value">{{ currentTempDisplay }}</strong>°C
              </p>
              <p class="ac__temps-line">
                Target:
                <strong class="ac__value">{{ targetTempDisplay }}</strong>°C
              </p>
              <p
                v-if="modeLabel"
                class="ac__temps-line"
              >
                Mode:
                <strong class="ac__value">{{ modeLabel }}</strong>
              </p>
              <p
                v-if="manualOverride"
                class="ac__temps-line"
              >
                Status:
                <strong class="ac__value">Manual mode</strong>
              </p>
            </div>

          <!-- Toggle: sends klimaON + manualOverride = true -->
          <label class="switch ac__switch">
            <input
              type="checkbox"
              :checked="isOn"
              @change="toggleManual"
            >
            <span class="slider round"></span>
          </label>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
/*
  Responsibilities:
  - Establish WebSocket connection to backend (wsStore.wsUrl).
  - Parse inbound messages for:
      channel === 'air_conditioning' → AC state updates (payload object)
      channel === 'room_stats'       → ambient temperature updates
  - Maintain reactive local state for display (currentTemp, requestedTemp, klimaON, etc.).
  - Provide user interactions:
      changeTemp(delta) → adjust requestTemp and send to WS
      toggleManual()    → toggle klimaON with manualOverride flag
  - Auto‑reconnect on socket close with a 5s backoff.
*/

import { ref, computed, onMounted, onUnmounted } from 'vue';
import { useWsStore } from '@/stores/wsStore';
import LoadingCard from '@/components/LoadingCard_component.vue';

const wsStore = useWsStore();

// WebSocket reference & reconnect timer
let ws = null;
let reconnectTimer;

// Reactive state from device/server
const currentTemp     = ref(null);   // Sensor current temperature (float)
const requestedTemp   = ref(null);   // Last requested target temperature (from device payload)
const inputTemp       = ref(22);     // Local temp UI control (optimistic update)
const klimaON         = ref(false);  // Device power state
const functionState   = ref('');     // Raw mode string from device ("CHLODZIMY!!!", etc.)
const manualOverride  = ref(false);  // Flag denoting manual control
const loading         = ref(true);   // Loading until first AC payload arrives

// Display helpers
const currentTempDisplay = computed(() =>
  currentTemp.value === null ? '--' : currentTemp.value.toFixed(1)
);
const targetTempDisplay = computed(() =>
  requestedTemp.value === null ? '--' : requestedTemp.value
);
const isOn = computed(() => klimaON.value);

// Map raw function codes to readable labels
const modeLabel = computed(() => {
  if (!functionState.value) return null;
  switch (functionState.value) {
    case 'CHLODZIMY!!!': return 'Cooling Mode';
    case 'GRZEJEMY!!!':  return 'Heating Mode';
    case 'W SPOCZYNKU':  return 'Idle Mode';
    default: return functionState.value;
  }
});

/*
  Establish WebSocket and set up event handlers.
  Re-establish connection if closed (5s delay).
*/
function connect() {
  loading.value = true;
  ws = new WebSocket(wsStore.wsUrl);

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);

      // AC state payload (preferred structure)
      if (data.channel === 'air_conditioning' && data.payload) {
        loading.value = false;
        updateAirConditioningState(data.payload);

        // Sync local inputTemp if server acknowledges requestedTemp
        if (typeof data.payload.requestedTemp === 'number') {
          inputTemp.value = data.payload.requestedTemp;
        }
      }
      // Ambient temperature broadcast from room_stats channel
      else if (data.channel === 'room_stats' && typeof data.temperature === 'number') {
        currentTemp.value = data.temperature;
      }
      // AC device disconnected indicator
      else if (data.channel === 'air_conditioning' && data.status === 'disconnected') {
        loading.value = true;
      }
    } catch {
      // Silently ignore malformed frames
    }
  };

  ws.onclose = () => {
    loading.value = true;
    scheduleReconnect();
  };

  ws.onerror = () => {
    loading.value = true;
    if (ws) ws.close(); // Force close to trigger reconnect flow
  };
}

/*
  Apply payload fields conditionally (avoid undefined overwrites).
*/
function updateAirConditioningState(payload) {
  if (payload.hasOwnProperty('klimaON'))         klimaON.value = payload.klimaON;
  if (payload.hasOwnProperty('currentTemp'))     currentTemp.value = payload.currentTemp;
  if (payload.hasOwnProperty('requestedTemp'))   requestedTemp.value = payload.requestedTemp;
  if (payload.hasOwnProperty('function'))        functionState.value = payload.function;
  if (payload.hasOwnProperty('manualOverride'))  manualOverride.value = payload.manualOverride;
}

/*
  Schedule a reconnect attempt (basic backoff).
*/
function scheduleReconnect() {
  if (reconnectTimer) clearTimeout(reconnectTimer);
  reconnectTimer = setTimeout(connect, 5000);
}

/*
  Adjust target temperature locally and dispatch command.
  Clamped between 16°C and 30°C (common AC safe range).
*/
function changeTemp(delta) {
  let newTemp = inputTemp.value + delta;
  if (newTemp < 16) newTemp = 16;
  if (newTemp > 30) newTemp = 30;
  inputTemp.value = newTemp;
  sendRequestedTemp(newTemp);
}

/*
  Send a temperature change request to the server (plaintext frontend payload).
  Server will forward encrypted message to AC device if needed.
*/
function sendRequestedTemp(temp) {
  const message = {
    channel: 'air_conditioning',
    payload: { requestedTemp: temp }
  };
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(message));
  }
}

/*
  Toggle manual power state.
  Sets manualOverride so device logic distinguishes user intent from automation.
*/
function toggleManual() {
  const newState = !klimaON.value;
  const message = {
    channel: 'air_conditioning',
    payload: {
      klimaON: newState,
      manualOverride: true
    }
  };
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(message));
  }
}

// Lifecycle: open connection and ensure clean teardown
onMounted(connect);
onUnmounted(() => {
  if (reconnectTimer) clearTimeout(reconnectTimer);
  if (ws) ws.close();
});
</script>

<style lang="scss" scoped>
/*
  Styling notes:
  - BEM block: .ac with nested elements.
  - card--ac kept for potential future card-level overrides.
  - Only structural/layout styles here; .btn & .switch rely on global styles.
*/
$ac-gap: 1rem;
$ac-accent: #333;

.ac {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 5rem;

  &__content {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: .8rem;
  }

  &__loading {
    width: 100%;
  }

  &__temp-form {
    display: flex;
    gap: $ac-gap;
    align-items: center;
    justify-content: center;
    margin-bottom: 1.2rem;
  }

  &__btn {
    // Inherits global .btn + .btn--square
  }

  &__temp-display {
    font-size: 1.5rem;
    min-width: 3.5rem;
    text-align: center;
    font-weight: 600;
  }

  &__temps {
    text-align: center;
    margin-bottom: .2rem;
    line-height: 1.3;

    &-line {
      margin: .15rem 0;
    }
  }

  &__value {
    color: $ac-accent;
  }

  &__switch {
    margin-top: .4rem;
    display: flex;
    justify-content: center;
  }
}

/* Card modifier placeholder for AC-specific future needs */
.card--ac {
  // (empty intentionally)
}

/* Slider variant from existing global layout */
.card__content--slider {
  height: fit-content;
}

/* Square button tweak (local override if global isn't sufficient) */
.btn--square {
  width: 2.5rem;
  height: 2.5rem;
  padding: 2rem !important;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 1.2rem;
  font-weight: 600;
}
</style>