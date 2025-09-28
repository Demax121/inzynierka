<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title">Main Door</h2>
    </div>
    <div class="card__body">
      <div class="card__content" v-if="door_sensor === 'Drzwi otwarte' || door_sensor === 'Drzwi zamknięte'">
        <div class="card__info-item">
          <span class="card__label">Status:</span>
          <span class="card__value">{{ door_status }}</span>
        </div>
        <div class="card__icon">
          <img :src="linkStore.getImage('lock-open.svg')" alt="Otwarta kłódka" class="door-icon"
            v-if="door_sensor === 'Drzwi otwarte'" />
          <img :src="linkStore.getImage('lock-closed.svg')" alt="Zamknięta kłódka" class="door-icon"
            v-else-if="door_sensor === 'Drzwi zamknięte'" />
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
import { useLinkStore } from '@/stores/linkStore';
import { useWsStore } from '@/stores/wsStore';
import { useDoorStatusStore } from '@/stores/doorStatusStore';
import LoadingCard from '@/components/LoadingCard.vue';

const linkStore = useLinkStore();
const wsStore = useWsStore();
const saveStore = useDoorStatusStore();
const door_status = ref();

const door_sensor = ref('Connecting...');
let ws;

onMounted(() => {
  ws = new WebSocket(wsStore.wsUrl);

  ws.onopen = () => {
    door_sensor.value = 'Waiting for data...';
  };

  ws.onclose = () => {
    if (door_sensor.value !== 'Drzwi otwarte' && door_sensor.value !== 'Drzwi zamknięte') {
      door_sensor.value = 'Connection error';
    }
  };

  ws.onerror = () => {
    if (door_sensor.value !== 'Drzwi otwarte' && door_sensor.value !== 'Drzwi zamknięte') {
      door_sensor.value = 'Connection error';
    }
  };

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === 'door_sensor' && typeof data.doorOpen === 'boolean') {
        door_sensor.value = data.doorOpen ? 'Drzwi otwarte' : 'Drzwi zamknięte';
        saveStore.saveDoorStatus(door_sensor.value);

        if (door_sensor.value === 'Drzwi otwarte'){
          door_status.value = 'Door open';
        }else{
          door_status.value = 'Door closed';
        }
      }
    } catch {
      // Ignoruj błędne pakiety
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