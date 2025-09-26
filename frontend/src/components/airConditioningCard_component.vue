<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title card__title--lights">Klimatyzacja</h2>
    </div>
    <div class="card__body">
      <div class="klima-wrapper">
        <LoadingCard v-if="loading" />
        <div v-else class="card__content card__content--slider">
          <div class="temp-form">
            <button type="button" class="temp-btn" @click="changeTemp(-1)">-1</button>
            <span class="temp-display">{{ inputTemp }}°C</span>
            <button type="button" class="temp-btn" @click="changeTemp(1)">+1</button>
          </div>
          <div class="temps-display">
            <p>Aktualna: <strong>{{ currentTempDisplay }}</strong>°C</p>
            <p>Docelowa: <strong>{{ targetTempDisplay }}</strong>°C</p>
            <p v-if="modeLabel">Funkcja: <strong>{{ modeLabel }}</strong></p>
            <p v-if="manualOverride">Status: <strong>Tryb ręczny</strong></p>
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
const inputTemp = ref(22); // domyślna wartość
const klimaON = ref(false);
const functionState = ref('');
const manualOverride = ref(false);
const loading = ref(true);

const currentTempDisplay = computed(() => currentTemp.value === null ? '--' : currentTemp.value);
const targetTempDisplay = computed(() => requestedTemp.value === null ? '--' : requestedTemp.value);
const isOn = computed(() => klimaON.value);

const modeLabel = computed(() => {
  if (!functionState.value) return null;
  
  switch(functionState.value) {
    case 'C': return 'Chłodzenie';
    case 'H': return 'Grzanie';
    case 'F': return 'Wentylator';
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
        // Synchronizuj inputTemp z docelową temperaturą
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
    currentTemp.value = payload.currentTemp;
  }
  if (payload.hasOwnProperty('requestedTemp')) {
    requestedTemp.value = payload.requestedTemp;
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
  let newTemp = inputTemp.value + delta;
  if (newTemp < 16) newTemp = 16;
  if (newTemp > 30) newTemp = 30;
  inputTemp.value = newTemp;
  sendRequestedTemp(newTemp);
}

function sendRequestedTemp(temp) {
  const message = {
    channel: "air_conditioning",
    payload: {
      requestedTemp: temp
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

.temp-btn {
  width: 2.5rem;
  height: 2.5rem;
  font-size: 1.5rem;
  padding: 0.5rem;
  border: 2px solid #ccc;

  background: #f5f5f5;
  cursor: pointer;
  transition: background 0.2s, border-color 0.2s;
  user-select: none;
}
.temp-btn:hover {
  background: #e0e0e0;
  border-color: #888;
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