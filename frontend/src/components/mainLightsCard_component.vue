<template>
  <!-- Main Lights Card: presents toggle for primary lighting over WebSocket channel 'main_lights' -->
  <div class="card main-lights">
    <div class="card__header">
      <h2 class="card__title card__title--lights">Main Lighting Control</h2>
    </div>
    <div class="card__body card__body--slider">
      <div class="card__content card__content--slider">
        <!-- Loading skeleton until first state sync arrives -->
        <LoadingCard v-if="loading" />
        <!-- Switch uses global .switch styles; checked state drives outbound command -->
        <label v-else class="switch switch--large" :aria-label="isLightOn ? 'Turn lights off' : 'Turn lights on'">
          <input type="checkbox" :checked="isLightOn" @change="toggleLights">
          <span class="slider round"></span>
        </label>
      </div>
    </div>
  </div>
</template>

<script setup>
// Handles bi-directional state for main lights over WebSocket.
// Inbound: { channel: 'main_lights', lightON: boolean }
// Outbound: { channel: 'main_lights', lightON: boolean }
// Auto-reconnects with a delay if the socket closes/errors.

import { ref, computed, onMounted, onUnmounted } from 'vue';
import { useWsStore } from '@/stores/wsStore';
import LoadingCard from '@/components/LoadingCard_component.vue';

const wsStore = useWsStore();

// Reactive state
const lightStatus = ref(false);   // Current known light state
const loading = ref(true);        // True until first valid message arrives
const isLightOn = computed(() => lightStatus.value === true);

// Connection handles
let ws = null; // WebSocket instance
let reconnectTimer = null; // reconnect timeout handle

// Configurable reconnect delay (ms) - adjust if needed
const RECONNECT_DELAY_MS = 5000;

// Send toggle command then optimistically update local state (UI stays responsive)
const toggleLights = () => {
  const newStatus = !lightStatus.value;
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({ channel: 'main_lights', lightON: newStatus }));
  }
  lightStatus.value = newStatus;
};

function connect() {
  ws = new WebSocket(wsStore.wsUrl);
  ws.onopen = () => { /* Passive until server broadcasts authoritative state */ };
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === 'main_lights' && typeof data.lightON === 'boolean') {
        lightStatus.value = data.lightON;
        loading.value = false;
      }
    } catch { /* ignore malformed message */ }
  };
  ws.onclose = () => { loading.value = true; scheduleReconnect(); };
  ws.onerror = () => { loading.value = true; if (ws) ws.close(); };
}

function scheduleReconnect() {
  if (reconnectTimer) clearTimeout(reconnectTimer);
  reconnectTimer = setTimeout(connect, RECONNECT_DELAY_MS);
}

onMounted(connect);
onUnmounted(() => {
  if (reconnectTimer) clearTimeout(reconnectTimer);
  if (ws) ws.close();
});
</script>

<style lang="scss" scoped>
// Block extension: main-lights (adds semantic hook)
// SCSS tokens
$ml-blink-duration: 1.4s;
$ml-blink-opacity-min: 0.25;
$ml-blink-opacity-max: 1;
$ml-loading-font-size: 3rem;

.card__body--slider { display:grid; justify-content:center; align-items:center; }
.card__content--slider { height:fit-content; }

// Legacy optional loading variant (kept if reused elsewhere)
.lights-loading {
  font-size:$ml-loading-font-size;
  line-height:1; text-align:center; opacity:.6;
  animation: blink $ml-blink-duration ease-in-out infinite;
}

@keyframes blink {
  0%,100% { opacity:$ml-blink-opacity-min; }
  50% { opacity:$ml-blink-opacity-max; }
}
</style>