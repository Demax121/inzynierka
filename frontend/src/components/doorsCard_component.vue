<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title">Main Door</h2>
    </div>
    <div class="card__body">
      <div class="card__content" v-if="door_sensor === 'Door open' || door_sensor === 'Door closed'">
        <div class="card__info-item">
          <span class="card__label">Status:</span>
          <span class="card__value">{{ door_sensor }}</span>
        </div>
        <div class="card__icon">
          <img :src="lockOpenIcon" alt="Door open" class="door-icon" v-if="door_sensor === 'Door open'" />
          <img :src="lockClosedIcon" alt="Door closed" class="door-icon" v-else-if="door_sensor === 'Door closed'" />
          <div class="door-icon door-icon--placeholder" v-else>
            <span>🔒</span>
          </div>
        </div>
      </div>
      <LoadingCard v-else />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue';
import { useWsStore } from '@/stores/wsStore';
import { useDoorStatusStore } from '@/stores/doorStatusStore';
import LoadingCard from '@/components/LoadingCard_component.vue';

import lockOpenIcon from '@/static assets/lock-open.svg';
import lockClosedIcon from '@/static assets/lock-closed.svg';

const wsStore = useWsStore();
const saveStore = useDoorStatusStore();

const door_sensor = ref('Connecting...');
let ws;

onMounted(() => {
  ws = new WebSocket(wsStore.wsUrl);

  ws.onopen = () => {
    door_sensor.value = 'Waiting for data...';
  };

  ws.onclose = () => {
    if (door_sensor.value !== 'Door open' && door_sensor.value !== 'Door closed') {
      door_sensor.value = 'Connection error';
    }
  };

  ws.onerror = () => {
    if (door_sensor.value !== 'Door open' && door_sensor.value !== 'Door closed') {
      door_sensor.value = 'Connection error';
    }
  };

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (
        data.channel === 'door_sensor' &&
        data.payload && typeof data.payload.doorOpen === 'boolean'
      ) {
        door_sensor.value = data.payload.doorOpen ? 'Door open' : 'Door closed';
        saveStore.saveDoorStatus(door_sensor.value);
      }
    } catch {
      console.error('Error parsing WebSocket message:', event.data);
    }
  };
});

onUnmounted(() => {
  if (ws) ws.close();
});
</script>

<style lang="scss" scoped>
$icon-size: 6rem;

.card__icon {
  display: flex;
  justify-content: center;
  align-items: center;
  margin-top: 3rem;
}

.door-icon {
  width: $icon-size;
  height: $icon-size;
  transition: transform 0.3s ease;

  &:hover {
    transform: scale(1.1);
  }

  &--placeholder {
    display: flex;
    justify-content: center;
    align-items: center;
    background: #f5f5f5;
    border-radius: 50%;
    color: #666;
    font-size: 24px;
  }
}
</style>