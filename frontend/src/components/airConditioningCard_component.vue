<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title card__title--lights">Air Conditioning</h2>
    </div>
    <div class="card__body">
      <div class="klima-wrapper">
        <LoadingCard v-if="loading" />
        <div v-else class="card__content card__content--slider">
          <div class="temp-form">
            <button type="button" class="btn btn--square" @click="changeTemp(-0.5)">-0.5</button>
            <span class="temp-display">{{ inputTemp }}°C</span>
            <button type="button" class="btn btn--square" @click="changeTemp(0.5)">+0.5</button>
          </div>

          <div class="temps-display">
            <p>Current: <strong>{{ currentTempDisplay }}</strong>°C</p>
            <p>Target: <strong>{{ targetTempDisplay }}</strong>°C</p>
            <p v-if="modeLabel">Mode: <strong>{{ modeLabel }}</strong></p>
            <p v-if="manualOverride">Status: <strong>Manual mode</strong></p>
          </div>
          <label class="switch">
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
import { ref, computed, onMounted, onUnmounted } from 'vue';
import { useWsStore } from '@/stores/wsStore';
import LoadingCard from '@/components/LoadingCard.vue';

const wsStore = useWsStore();

let ws = null;
let reconnectTimer;

// State variables
const currentTemp = ref(null);
const requestedTemp = ref(null);
const inputTemp = ref(22.0); // default value (float)
const klimaON = ref(false);
const functionState = ref('');
const manualOverride = ref(false);
const loading = ref(true);

const currentTempDisplay = computed(() => {
  if (currentTemp.value === null || isNaN(currentTemp.value)) return '--';
  return (Math.round(Number(currentTemp.value) * 10) / 10).toFixed(1);
});
const targetTempDisplay = computed(() => {
  if (requestedTemp.value === null || isNaN(requestedTemp.value)) return '--';
  return (Math.round(Number(requestedTemp.value) * 10) / 10).toFixed(1);
});
const isOn = computed(() => klimaON.value);

const modeLabel = computed(() => {
  if (!functionState.value) return null;
  
  switch(functionState.value) {
    case 'CHLODZIMY!!!': return 'Cooling Mode';
    case 'GRZEJEMY!!!': return 'Heating Mode';
    case 'W SPOCZYNKU': return 'Idle Mode';
    default: return functionState.value;
  }
});

function connect() {
  loading.value = true;
  ws = new WebSocket(wsStore.wsUrl);
  
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === 'air_conditioning' && data.payload) {
        loading.value = false;
        updateAirConditioningState(data.payload);
        // Synchronize inputTemp with target temperature
        if (typeof data.payload.requestedTemp === 'number') {
          inputTemp.value = data.payload.requestedTemp;
        }
      } else if (data.channel === 'room_stats' && typeof data.temperature === 'number') {
        currentTemp.value = data.temperature;
      } else if (data.channel === 'air_conditioning' && data.status === 'disconnected') {
        loading.value = true;
      }
    } catch {}
  };
  
  ws.onclose = () => {
    loading.value = true;
    scheduleReconnect();
  };
  
  ws.onerror = () => {
    loading.value = true;
    if (ws) ws.close();
  };
}

function updateAirConditioningState(payload) {
  if (payload.hasOwnProperty('klimaON')) {
    klimaON.value = payload.klimaON;
  }
  if (payload.hasOwnProperty('currentTemp')) {
    currentTemp.value = parseFloat(payload.currentTemp);
  }
  if (payload.hasOwnProperty('requestedTemp')) {
    requestedTemp.value = parseFloat(payload.requestedTemp);
  }
  if (payload.hasOwnProperty('function')) {
    functionState.value = payload.function;
  }
  if (payload.hasOwnProperty('manualOverride')) {
    manualOverride.value = payload.manualOverride;
  }
}

function scheduleReconnect() {
  if (reconnectTimer) clearTimeout(reconnectTimer);
  reconnectTimer = setTimeout(connect, 5000);
}

function changeTemp(delta) {
  let newTemp = parseFloat(inputTemp.value) + parseFloat(delta);
  if (newTemp < 16.0) newTemp = 16.0;
  if (newTemp > 30.0) newTemp = 30.0;
  inputTemp.value = parseFloat(newTemp.toFixed(1));
  sendRequestedTemp(inputTemp.value);
}

function sendRequestedTemp(temp) {
  const message = {
    channel: "air_conditioning",
    payload: {
      requestedTemp: parseFloat(temp)
    }
  };
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(message));
  }
}

function toggleManual() {
  const newState = !klimaON.value;
  const message = {
    channel: "air_conditioning", 
    payload: {
      klimaON: newState,
      manualOverride: true
    }
  };
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(message));
  }
}

onMounted(connect);
onUnmounted(() => { if (reconnectTimer) clearTimeout(reconnectTimer); if (ws) ws.close(); });
</script>

<style lang="scss" scoped>
.klima-wrapper { 
  display: flex; 
  justify-content: center; 
  align-items: center; 
  min-height: 5rem; 
}

.temp-form {
  display: flex;
  gap: 1rem;
  align-items: center;
  justify-content: center;
  margin-bottom: 1.5rem;
}

.btn--square {
  width: 2.5rem;
  height: 2.5rem;
  padding: 2rem !important; // force equal sides
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 1.2rem; // adjust as needed
  font-weight: 600;
}

.temp-display {
  font-size: 1.5rem;
  min-width: 3.5rem;
  text-align: center;
  font-weight: 600;
}

.temps-display { 
  text-align: center; 
  margin-bottom: .2rem; 
  line-height: 1.3; 
}

.temps-display p { 
  margin: .15rem 0; 
}

.loading-wrapper { 
  font-size: 2rem; 
  opacity: .6; 
  animation: pulse 1.4s ease-in-out infinite; 
  text-align: center;
  padding: 2rem;
}

@keyframes pulse { 
  0%, 100% { opacity: .3; }
  50% { opacity: 1; }
}

.card__content--slider {
  height: fit-content;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  gap: 0.8rem;
}
</style>