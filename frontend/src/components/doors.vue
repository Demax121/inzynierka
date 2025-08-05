<template>
  <div class="card-container">
    <h2 class="card__title">Główne drzwi</h2>
    <div class="device-info">
      <p>Status: {{ doorStatus }}</p>
    </div>
  </div>
</template>
<script setup>
import { ref, onMounted, onUnmounted } from 'vue';

const doorStatus = ref('Brak danych');
let ws;

onMounted(() => {
  ws = new WebSocket('ws://192.168.1.4:8886'); // Adres serwera WebSocket

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === "doorStatus") {
        doorStatus.value = data.status === "otwarte" ? "Drzwi otwarte" : "Drzwi zamknięte";
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

<style lang="scss" scoped></style>