<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title card__title--lights">Klimatyzacja</h2>
    </div>
    <div class="card__body">
      <div class="klima-wrapper">
        <div v-if="loading" class="klima-loading">💨</div>
        <div v-else class="card__content card__content--slider">
          <form class="temp-form" @submit.prevent="submitTemp">
            <input v-model.number="inputTemp" type="number" min="16" max="30" step="1" placeholder="Wprowadź temperaturę" class="temp-input" required />
            <button type="submit" class="btn">Ustaw temperaturę</button>
          </form>
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

let ws = null;
let reconnectTimer;

// State variables
const currentTemp = ref(null);
const requestedTemp = ref(null);
const inputTemp = ref("");
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
  ws = new WebSocket('ws://192.168.1.4:8886');
  
  // ws.onopen = () => {
  // };
  
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      
      // Obsługa payload format (nowy)
      if (data.channel === 'air_conditioning' && data.payload) {
        loading.value = false; // Set loading to false when we receive device data
        updateAirConditioningState(data.payload);
      }
      // Obsługa room_stats dla temperatury
      else if (data.channel === 'room_stats' && typeof data.temperature === 'number') {
        currentTemp.value = data.temperature;
        console.log(`Zaktualizowano temperaturę z room_stats: ${data.temperature}°C`);
      }
      // Obsługa disconnection status
      else if (data.channel === 'air_conditioning' && data.status === 'disconnected') {
        console.log('ESP32 air_conditioning rozłączony');
        loading.value = true; // Set loading back to true when device disconnects
      }
    } catch (error) {
      console.error('Błąd parsowania danych klimatyzacji:', error);
    }
  };
  
  ws.onclose = () => {
    console.log('WebSocket połączenie zamknięte dla klimatyzacji');
    loading.value = true;
    scheduleReconnect();
  };
  
  ws.onerror = (error) => {
    console.error('Błąd WebSocket dla klimatyzacji:', error);
    loading.value = true;
    if (ws) ws.close();
  };
}

function updateAirConditioningState(payload) {
  // loading już wyłączony w onopen
  
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

function submitTemp() {
  if (!inputTemp.value || inputTemp.value < 16 || inputTemp.value > 30) return;
  
  const message = {
    channel: "air_conditioning",
    payload: {
      requestedTemp: inputTemp.value
    }
  };
  
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(message));
    inputTemp.value = "";
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
.klima-wrapper { display:flex; justify-content:center; align-items:center; min-height:5rem; }
.klima-loading { font-size: 2.5rem; opacity: .6; animation: bounce 1.4s ease-in-out infinite; }
@keyframes bounce { 
  0%, 100% { transform: translateY(0); opacity:.3; } 
  50% { transform: translateY(-10px); opacity:1; } 
}

.temp-form {
  display: flex;
  gap: 1rem;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  margin-bottom: 1.5rem;
}

.temps-display { text-align:center; margin-bottom: .2rem; line-height:1.3; }
.temps-display p { margin:.15rem 0; }

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

.temp-input {
  padding: 8px;
  font-size: 12pt;
  width: 14rem;
  border: 2px solid #ccc;
  border-radius: 4px;
  outline: none;
  transition: border-color 0.3s;
  display: block;
}

.temp-input:focus { border-color: var(--color); }

$slider-switch-size: 2.5rem;

.card__content--slider {
  height: fit-content;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  gap: 0.8rem;
}

/* The switch - the box around the slider */
.switch { position: relative; display: inline-block; width: 8rem; height: 3rem; }

/* Hide default HTML checkbox */
.switch input { opacity: 0; width: 0; height: 0; }

/* The slider */
.slider {
  position: absolute;
  cursor: pointer; // ZMIANA: było not-allowed
  top: 0; left: 0; right: 0; bottom: 0;
  background-color: #222;
  border-radius: 34px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 1.4rem;
  font-weight: bold;
  color: white;
  transition: background-color .4s, box-shadow .4s;
  overflow: hidden;
}

/* Domyślny tekst OFF */
.slider::after { content: "OFF"; opacity: 1; transition: opacity .3s ease-in-out; position: relative; z-index: 2; }

/* Wewnętrzne kółko */
.slider:before {
  position: absolute;
  content: "";
  height: $slider-switch-size;
  width: $slider-switch-size;
  left: 4px; bottom: 4px;
  background-color: white;
  border-radius: 50%;
  transition: .4s; z-index: 1;
}

/* Efekt glow przy ON */
input:checked + .slider { box-shadow: 0 0 25px 6px #d40fe6; }

/* Zmiana tekstu ON/OFF z fade */
input:checked + .slider::after { content: "ON"; opacity: 1; animation: fadeText .3s ease-in-out; }

/* Gdy zaznaczony przesuwamy kółko w prawo */
input:checked + .slider:before { transform: translateX(5rem); }

/* Animacja fade dla tekstu */
@keyframes fadeText { 0% { opacity: 0; } 50% { opacity: .5; } 100% { opacity: 1; } }
</style>