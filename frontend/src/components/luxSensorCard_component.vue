<template>
  <!-- Light Sensor Card: shows current lux value via WebSocket (channel: 'lux_sensor') -->
  <div class="card lux-sensor">
    <div class="card__header">
      <h2 class="card__title">Light Sensor</h2>
    </div>
    <div class="card__body">
      <div class="lux-sensor__wrapper">
        <!-- Loading skeleton until first value arrives -->
        <LoadingCard v-if="lux === null" />
        <!-- Display numeric lux value once received -->
        <div v-else class="lux-sensor__value">
          {{ lux }} <span class="lux-sensor__unit">lux</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
// Displays live ambient light (lux) fed over WebSocket.
// Lifecycle: open WS on mount, parse messages, close on unmount.
// Messages shape (from bunServer): { channel: 'lux_sensor', lux: <number> }

import { ref, onMounted, onUnmounted } from 'vue';
import { useWsStore } from '@/stores/wsStore';
import LoadingCard from '@/components/LoadingCard_component.vue';

const wsStore = useWsStore();

// Reactive current lux value; null indicates not yet received.
const lux = ref(null);
let ws; // WebSocket instance (scoped for cleanup)

onMounted(() => {
  ws = new WebSocket(wsStore.wsUrl);
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === 'lux_sensor') lux.value = data.lux;
    } catch { /* Ignore malformed messages */ }
  };
});

onUnmounted(() => { if (ws) ws.close(); });
</script>

<style scoped lang="scss">
// Block: .lux-sensor (local namespaced styles to avoid collisions)
// Design tokens
$lux-wrapper-min-height: 5rem;
$lux-wrapper-margin-top: 4rem;
$lux-value-font-size: 4rem;
$lux-unit-font-size: 2rem;
$lux-unit-margin-left: 0.4rem;
$lux-unit-opacity: 0.7;
$lux-value-color: #000;

.lux-sensor {
  &__wrapper {
    display:flex;
    justify-content:center;
    align-items:center;
    min-height:$lux-wrapper-min-height;
    margin-top:$lux-wrapper-margin-top;
  }
  &__value {
    font-size:$lux-value-font-size;
    font-weight:600;
    color:$lux-value-color;
    line-height:1;
    letter-spacing:-0.02em;
  }
  &__unit {
    font-size:$lux-unit-font-size;
    margin-left:$lux-unit-margin-left;
    opacity:$lux-unit-opacity;
    font-weight:400;
  }
}
</style>
