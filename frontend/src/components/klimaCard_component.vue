<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title card__title--lights">Klimatyzacja</h2>
    </div>
    <div class="card__body card__body--slider">
      <div class="card__content card__content--slider">
        <form class="temp-form" @submit.prevent="submitTemp">
          <input v-model.number="inputTemp" type="number" min="16" max="30" step="1" placeholder="Wprowadź temperaturę" class="temp-input" required />
          <button type="submit" class="btn">Ustaw temperaturę</button>
        </form>
        <div class="temps-display" v-if="!loading">
          <p>Aktualna: <strong>{{ currentTempDisplay }}</strong>°C</p>
          <p>Docelowa: <strong>{{ targetTempDisplay }}</strong>°C</p>
          <p v-if="modeLabel">Funkcja: <strong>{{ modeLabel }}</strong></p>
        </div>
        <div v-else class="temps-loading">⏳</div>
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
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue';

let ws = null;
let reconnectTimer;

// Temperatures
const currentTemp = ref(null); // aktualna z roomStats / status
const targetTemp = ref(25);    // default value
const inputTemp = ref(25);
const klimaStatus = ref('OFF'); // ON / OFF
const mode = ref('idle');       // cooling / heating / idle
const currentFunction = ref(''); // funkcja otrzymana z ESP32
const loading = ref(true);

const currentTempDisplay = computed(() => currentTemp.value === null ? '-' : currentTemp.value);
const targetTempDisplay = computed(() => targetTemp.value === null ? '-' : targetTemp.value);
const isOn = computed(() => klimaStatus.value === 'ON');

const modeLabel = computed(() => {
  if (!isOn.value) return null; // Nie pokazuj statusu gdy OFF
  
  // Używamy funkcji otrzymanej z ESP32
  return currentFunction.value || 'IDLE';
});

let lastCommandTs = 0;
const COMMAND_DEBOUNCE_MS = 800; // aby nie spamować przy szybkim klikaniu
function sendManual(status) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({ channel: 'klimatyzacja', klimaStatus: status }));
  }
}

function toggleManual() {
  const now = Date.now();
  if (now - lastCommandTs < COMMAND_DEBOUNCE_MS) return;
  lastCommandTs = now;
  const next = isOn.value ? 'OFF' : 'ON';
  klimaStatus.value = next; // optymistycznie
  sendManual(next);
}

function connect() {
  ws = new WebSocket('ws://192.168.1.4:8886');
  ws.onopen = () => {
    // czekamy na status z ESP32 albo roomStats
  };
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === 'klimatyzacja') {
        // Sprawdź czy ESP32 się rozłączył
        if (data.status === 'disconnected') {
          loading.value = true;
          return;
        }
        
        // jeśli serwer dośle temperature (ambient)
        if (Number.isFinite(data.temperature)) currentTemp.value = Math.round(data.temperature);
        if (Number.isInteger(data.currentTemp)) currentTemp.value = data.currentTemp; // kompatybilność gdyby później zmieniono nazwę
        if (Number.isInteger(data.targetTemp)) {
          targetTemp.value = data.targetTemp;
          inputTemp.value = data.targetTemp;
        }
        if (typeof data.klimaStatus === 'string') klimaStatus.value = data.klimaStatus;
        if (typeof data.mode === 'string') mode.value = data.mode;
        if (typeof data.currentFunction === 'string') currentFunction.value = data.currentFunction;
        loading.value = false;
      }
      if (data.channel === 'roomStats' && Number.isFinite(data.temperature)) {
        currentTemp.value = Math.round(data.temperature);
        loading.value = false;
      }
    } catch {}
  };
  ws.onclose = () => { loading.value = true; scheduleReconnect(); };
  ws.onerror = () => { loading.value = true; if (ws) ws.close(); };
}

function scheduleReconnect() {
  if (reconnectTimer) clearTimeout(reconnectTimer);
  reconnectTimer = setTimeout(connect, 5000);
}

function submitTemp() {
  if (!Number.isInteger(inputTemp.value)) return;
  if (inputTemp.value < 16 || inputTemp.value > 30) return;
  
  targetTemp.value = inputTemp.value; // optimistic update
  
  console.log(`Sending requestedTemp: ${targetTemp.value}`); // Debug log
  
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({ 
      channel: 'klimatyzacja', 
      requestedTemp: targetTemp.value 
    }));
    console.log(`Sent: {"channel": "klimatyzacja", "requestedTemp": ${targetTemp.value}}`); // Debug log
  } else {
    console.log('WebSocket not connected'); // Debug log
  }
}

onMounted(connect);
onUnmounted(() => { if (reconnectTimer) clearTimeout(reconnectTimer); if (ws) ws.close(); });
</script>

<style lang="scss" scoped>
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
.temps-loading { font-size:2rem; opacity:.6; animation: pulse 1.4s ease-in-out infinite; }
@keyframes pulse { 0%,100% { opacity:.3;} 50% { opacity:1;} }

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

.card__body--slider { display: grid; justify-content: center; align-items: center; }

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