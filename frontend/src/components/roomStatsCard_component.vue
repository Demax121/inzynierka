<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Room Statistics</h2>
        </div>
        <div class="card__body">
            <div class="card__content" v-if="statsInfo">
                <div class="card__info-item">
                    <span class="card__label">Temperature:</span>
                    <span class="card__value">{{ formatTemperature(statsInfo.temperature) }}</span>
                </div>
                <div class="card__info-item">
                    <span class="card__label">Humidity:</span>
                    <span class="card__value">{{ formatHumidity(statsInfo.humidity) }}</span>
                </div>
                <div class="card__info-item">
                    <span class="card__label">Pressure:</span>
                    <span class="card__value">{{ formatPressure(statsInfo.pressure) }}</span>
                </div>
                <!-- <button @click="saveDatas">Save statistics</button> -->
            </div>
            <LoadingCard v-else />
        </div>
    </div>
</template>



<script setup>
import { ref, onMounted, onUnmounted } from 'vue';
import { useWsStore } from '@/stores/wsStore';
const wsStore = useWsStore();

import { useSaveStatsStore } from '@/stores/saveStatsStore'
const saveStore = useSaveStatsStore()

import LoadingCard from '@/components/LoadingCard_component.vue'

function saveDatas(){
  saveStore.forceSaveNow();
}

const statsInfo = ref(null);
let ws;

// Formatting functions
const formatTemperature = (temp) => {
  if (typeof temp === 'number') {
    return `${temp.toFixed(1)} °C`;
  }
  return 'N/A';
};

const formatHumidity = (humidity) => {
  if (typeof humidity === 'number') {
    return `${humidity.toFixed(0)} %`;
  }
  return 'N/A';
};

const formatPressure = (pressure) => {
  if (typeof pressure === 'number') {
    return `${pressure.toFixed(0)} hPa`;
  }
  return 'N/A';
};

onMounted(() => {
  ws = new WebSocket(wsStore.wsUrl);
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === 'room_stats') {
        statsInfo.value = { 
          temperature: data.temperature, 
          humidity: data.humidity, 
          pressure: data.pressure 
        };
        saveStore.setStats({ temperature: data.temperature, humidity: data.humidity, pressure: data.pressure })
      }
    } catch {}
  };
  ws.onclose = () => { if (!statsInfo.value) statsInfo.value = null; };
  ws.onerror = () => { if (!statsInfo.value) statsInfo.value = null; };
});

onUnmounted(() => { if (ws) ws.close(); });
</script>



<style lang="scss" scoped>
.card__content--loading { display: flex; flex-direction: column; align-items: center; gap: .75rem; }
.loading-icon { font-size: 2.5rem; animation: float 2s ease-in-out infinite; }
@keyframes float { 0%,100%{ transform: translateY(0);} 50%{ transform: translateY(-6px);} }
</style>