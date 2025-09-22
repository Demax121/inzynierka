<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title">Natężenie światła</h2>
    </div>
    <div class="card__body">
      <div class="lux-wrapper">
        <div v-if="lux === null" class="lux-loading">☀️</div>
        <div v-else class="lux-value">{{ lux }} <span class="unit">lux</span></div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue';

const lux = ref(null);
let ws;

onMounted(() => {
  ws = new WebSocket('ws://192.168.1.4:8886');
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === 'lux_sensor') {
        lux.value = data.lux;
      }
    } catch {}
  };
});

onUnmounted(() => { if (ws) ws.close(); });
</script>

<style scoped>
.lux-wrapper { display:flex; justify-content:center; align-items:center; min-height:5rem; }
.lux-loading { font-size:2.5rem; opacity:.6; animation: blink 1.4s ease-in-out infinite; }
@keyframes blink { 0%,100% { opacity:.3; } 50% { opacity:1; } }
.lux-value { font-size:2rem; font-weight:600; color:black; }
.unit { font-size:2rem; margin-left:.4rem; opacity:.7; }
</style>
