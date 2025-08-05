<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Statystyki pomieszczenia</h2>
        </div>
        <div class="card__body">
            <div class="card__content" v-if="statsInfo">
                <div class="card__info-item">
                    <span class="card__label">Temperatura:</span>
                    <span class="card__value">{{ statsInfo.temperature }}</span>
                </div>
                <div class="card__info-item">
                    <span class="card__label">Wilgotność:</span>
                    <span class="card__value">{{ statsInfo.humidity }}</span>
                </div>
                <div class="card__info-item">
                    <span class="card__label">Ciśnienie:</span>
                    <span class="card__value">{{ statsInfo.pressure }}</span>
                </div>
            </div>
            <div class="card__content" v-else>
                <p class="card__text">Brak danych z czujników</p>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue';
import { useLinkStore } from '@/stores/linkStore';
const linkStore = useLinkStore();

const statsInfo = ref(null);
let ws;

// Function to fetch initial room stats via HTTP
const fetchInitialStats = async () => {
  try {
    const response = await fetch(linkStore.getPhpApiUrl('roomStats.php'));
    const data = await response.json();
    if (data.temperature && data.humidity && data.pressure) {
      statsInfo.value = {
        temperature: data.temperature,
        humidity: data.humidity,
        pressure: data.pressure
      };
    }
  } catch (error) {
    console.error('Failed to fetch initial room stats:', error);
    statsInfo.value = null;
  }
};

onMounted(() => {
  ws = new WebSocket('ws://192.168.1.4:8886');
  
  ws.onopen = () => {
    fetchInitialStats();
  };
  
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
      console.error('Room stats JSON parse error:', e);
    }
  };

  ws.onclose = () => {
    statsInfo.value = null;
  };

  ws.onerror = (error) => {
    console.error('Room stats WebSocket error:', error);
    statsInfo.value = null;
  };
});

onUnmounted(() => {
  if (ws) ws.close();
});
</script>

<style lang="scss" scoped>
</style>