<template>
  <!-- Room statistics card: live temperature, humidity, pressure from WS channel 'room_stats' -->
  <div class="card room-stats">
    <div class="card__header">
      <h2 class="card__title">Room Statistics</h2>
    </div>
    <div class="card__body">
      <!-- Content section: only visible once first payload arrives -->
      <div class="card__content room-stats__content" v-if="statsInfo">
        <div class="card__info-item room-stats__row">
          <span class="card__label room-stats__label">Temperature:</span>
          <span class="card__value room-stats__value">{{ formatTemperature(statsInfo.temperature) }}</span>
        </div>
        <div class="card__info-item room-stats__row">
          <span class="card__label room-stats__label">Humidity:</span>
          <span class="card__value room-stats__value">{{ formatHumidity(statsInfo.humidity) }}</span>
        </div>
        <div class="card__info-item room-stats__row">
          <span class="card__label room-stats__label">Pressure:</span>
          <span class="card__value room-stats__value">{{ formatPressure(statsInfo.pressure) }}</span>
        </div>
      </div>
      <!-- Loading state -->
      <!-- Manual save button (visible when we have stats) -->
      <!-- <div class="card__actions" v-if="statsInfo">
        <button
          class="btn room-stats__save-btn"
          @click="saveNow"
          :disabled="saving"
        >
          <span v-if="!saving">Save now</span>
          <span v-else>Saving…</span>
        </button>
      </div> -->
      <LoadingCard v-else />
    </div>
  </div>
</template>



<script setup>
// Subscribes to WebSocket channel 'room_stats' and displays latest environmental readings.
// Also forwards values to saveStatsStore for periodic backend persistence.
import { ref, onMounted, onUnmounted } from 'vue';
import { useWsStore } from '@/stores/wsStore';
import { useSaveStatsStore } from '@/stores/saveStatsStore';
import LoadingCard from '@/components/LoadingCard_component.vue';

const wsStore = useWsStore();
const saveStore = useSaveStatsStore();

// Latest stats object: { temperature, humidity, pressure }
const statsInfo = ref(null);
let ws; // WebSocket instance
const saving = ref(false);

// Formatting helpers (kept pure & defensive)
const formatTemperature = (temp) => (typeof temp === 'number' ? `${temp.toFixed(1)} °C` : 'N/A');
const formatHumidity    = (hum)  => (typeof hum === 'number'  ? `${hum.toFixed(0)} %`   : 'N/A');
const formatPressure    = (prs)  => (typeof prs === 'number'  ? `${prs.toFixed(0)} hPa` : 'N/A');

onMounted(() => {
  ws = new WebSocket(wsStore.wsUrl);
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === 'room_stats') {
        statsInfo.value = { temperature: data.temperature, humidity: data.humidity, pressure: data.pressure };
        // Sync into store for scheduled saving logic
        saveStore.setStats({ temperature: data.temperature, humidity: data.humidity, pressure: data.pressure });
      }
    } catch { /* ignore malformed */ }
  };
  // On connection issues keep previously displayed values; only set null if never received any
  ws.onclose = () => { if (!statsInfo.value) statsInfo.value = null; };
  ws.onerror = () => { if (!statsInfo.value) statsInfo.value = null; };
});

onUnmounted(() => { if (ws) ws.close(); });

// Manual save trigger for user
const saveNow = async () => {
  if (!statsInfo.value) return
  if (saving.value) return
  saving.value = true
  try {
    await saveStore.forceSaveNow()
  } catch (e) {
    // Minimal UX feedback: log error
    console.error('Manual save failed', e)
  } finally {
    // small delay so UI feels responsive even on very fast ops
    setTimeout(() => { saving.value = false }, 300)
  }
}
</script>



<style lang="scss" scoped>
// Block: room-stats (extends generic card info layout)
// Tokens
$rs-row-gap: 2rem;
$rs-label-color: #555;
$rs-value-color: #111;
$rs-label-font-weight: 500;
$rs-value-font-weight: 600;
$rs-transition: 0.3s ease;
$label-font-size: 14pt;
$value-font-size: 14pt;

.room-stats {
  &__content { display:flex; flex-direction:column; gap:$rs-row-gap; }
  &__row { display:flex; justify-content:space-between; align-items:baseline; }
  &__label { font-weight:$rs-label-font-weight; color:$rs-label-color; font-size: $label-font-size;}
  &__value { font-weight:$rs-value-font-weight; color:$rs-value-color; letter-spacing:0.5px; font-size: $value-font-size };
  &__save-btn {
    padding: 0.5rem 1rem;
    background: #1976d2;
    color: white;
    border: none;
    border-radius: 6px;
    cursor: pointer;
    transition: opacity $rs-transition;
  }
  &__save-btn:disabled { opacity: 0.6; cursor: default }
}

</style>