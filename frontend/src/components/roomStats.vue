<template>
    <div class="card-container">
        <h2 class="card__title">Statystyki pomieszczenia</h2>
        <div class="device-info" v-if="statsInfo">
            <p>Temperatura: {{ statsInfo.temperature }}</p>
            <p>Wilgotność: {{ statsInfo.humidity }}</p>
            <p>Ciśnienie: {{ statsInfo.pressure }}</p>
        </div>
        <div v-else>
            <p>Brak danych</p>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue';

const statsInfo = ref(null);
let ws;

onMounted(() => {
  ws = new WebSocket('ws://192.168.1.4:8886');
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === "roomStats") {
        statsInfo.value = {
          temperature: data.temperature,
          humidity: data.humidity,
          pressure: data.pressure
        };
      }
    } catch (e) {
      // ignoruj błędny JSON
    }
  };
});

onUnmounted(() => {
  if (ws) ws.close();
});
</script>

<style lang="scss" scoped>
</style>