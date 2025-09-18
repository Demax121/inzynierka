<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title card__title--lights">Sterowanie głównym oświetleniem</h2>
    </div>
    <div class="card__body card__body--slider">
      <div class="card__content card__content--slider">
        <div v-if="loading" class="lights-loading">💡</div>
        <label v-else class="switch switch--large">
          <input type="checkbox" :checked="isLightOn" @change="toggleLights">
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
    } catch { }
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
.card__body--slider {
  display: grid;
  justify-content: center;
  align-items: center;
}

.card__content--slider {
  height: fit-content;
}

.lights-loading {
  font-size: 3rem;
  line-height: 1;
  text-align: center;
  opacity: .6;
  animation: blink 1.4s ease-in-out infinite;
}

@keyframes blink {

  0%,
  100% {
    opacity: .25;
  }

  50% {
    opacity: 1;
  }
}
</style>