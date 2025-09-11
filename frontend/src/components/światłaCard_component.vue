<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title card__title--lights">Sterowanie głównym oświetleniem</h2>
        </div>
        <div class="card__body card__body--slider">
            <div class="card__content card__content--slider">
                <div v-if="loading" class="lights-loading">💡</div>
                <label v-else class="switch">
                  <input 
                    type="checkbox" 
                    :checked="isLightOn" 
                    @change="toggleLights"
                  >
                  <span class="slider round"></span>
                </label>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue';
import { useLinkStore } from '@/stores/linkStore';
const linkStore = useLinkStore();

const lightStatus = ref(false);
const loading = ref(true);
const isLightOn = computed(() => lightStatus.value === true);
let ws = null;
let reconnectTimer;

const toggleLights = () => {
  const newStatus = !lightStatus.value;
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({ channel: 'main_lights', lightON: newStatus }));
  }
  lightStatus.value = newStatus;
};

function connect() {
  ws = new WebSocket('ws://192.168.1.4:8886');
  ws.onopen = () => { /* czekamy na pierwszą wiadomość push */ };
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === 'main_lights' && typeof data.lightON === 'boolean') {
        lightStatus.value = data.lightON;
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

onMounted(connect);
onUnmounted(() => { if (reconnectTimer) clearTimeout(reconnectTimer); if (ws) ws.close(); });
</script>

<style lang="scss" scoped>

$slider-switch-size: 3.5rem;

.card__body--slider {
  display: grid;
  justify-content: center;
  align-items: center;
}

.card__content--slider {
  height: fit-content;
}

.lights-loading { font-size: 3rem; line-height: 1; text-align:center; opacity:.6; animation: blink 1.4s ease-in-out infinite; }
@keyframes blink { 0%,100% { opacity:.25; } 50% { opacity:1; } }

/* The switch - the box around the slider */
.switch {
  position: relative;
  display: inline-block;
  width: 11rem;
  height: 4rem;
}

/* Hide default HTML checkbox */
.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

/* The slider */
.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
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
.slider::after {
  content: "OFF";
  opacity: 1;
  transition: opacity .3s ease-in-out;
  position: relative;
  z-index: 2;
}

/* Wewnętrzne kółko */
.slider:before {
  position: absolute;
  content: "";
  height: $slider-switch-size;
  width: $slider-switch-size;
  left: 4px;
  bottom: 4px;
  background-color: white;
  border-radius: 50%;
  transition: .4s;
  z-index: 1;
}

/* Efekt glow przy ON */
input:checked + .slider {
  box-shadow: 0 0 25px 6px #d40fe6; /* glow */
}

/* Zmiana tekstu ON/OFF z fade */
input:checked + .slider::after {
  content: "ON";
  opacity: 1;
  animation: fadeText .3s ease-in-out;
}

/* Gdy zaznaczony przesuwamy kółko w prawo */
input:checked + .slider:before {
  transform: translateX(7rem);
}

/* Animacja fade dla tekstu */
@keyframes fadeText {
  0% { opacity: 0; }
  50% { opacity: 0.5; }
  100% { opacity: 1; }
}




</style>