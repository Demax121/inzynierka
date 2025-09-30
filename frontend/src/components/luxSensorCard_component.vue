<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title">Light Sensor</h2>
    </div>
    <div class="card__body">
      <div class="lux-wrapper">
        <LoadingCard v-if="lux === null" />
        <div v-else class="lux-value">{{ lux }} <span class="unit">lux</span></div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue';
import { useWsStore } from '@/stores/wsStore';
import LoadingCard from '@/components/LoadingCard.vue';

const wsStore = useWsStore(); // Dodaj inicjalizację store'a

const lux = ref(null);
let ws;

onMounted(() => {
  ws = new WebSocket(wsStore.wsUrl); // Zmieniono na dynamiczny adres
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


.lux-wrapper { display:flex; justify-content:center; align-items:center; min-height:5rem; margin-top: 4rem; }
.lux-value { font-size:4rem; font-weight:600; color:black; }
.unit { font-size:2rem; margin-left:.4rem; opacity:.7; }
</style>
