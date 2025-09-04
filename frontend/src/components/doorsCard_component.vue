<template>
  <div class="card">
    <div class="card__header">
      <h2 class="card__title">Główne drzwi</h2>
    </div>
    <div class="card__body">
      <div class="card__content">
        <div class="card__info-item">
          <span class="card__label">Status:</span>
          <span class="card__value">{{ doorStatus }}</span>
        </div>
        <div class="card__icon">
          <img :src="linkStore.getImage('lock-open.svg')" alt="Otwarta kłódka" class="door-icon"
            v-if="doorStatus === 'Drzwi otwarte'" />
          <img :src="linkStore.getImage('lock-closed.svg')" alt="Zamknięta kłódka" class="door-icon"
            v-else-if="doorStatus === 'Drzwi zamknięte'" />
          <div class="door-icon door-icon--placeholder" v-else>
            <span>🔒</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>
<script setup>
import { ref, onMounted, onUnmounted } from 'vue';
import { useLinkStore } from '@/stores/linkStore';
const linkStore = useLinkStore();

// Stan początkowy utrzymany aż do pierwszej wiadomości z ESP
const doorStatus = ref('Łączenie...');
let ws;

onMounted(() => {
  ws = new WebSocket('ws://192.168.1.4:8886');

  ws.onopen = () => {
    // Po nawiązaniu połączenia czekamy na pierwszą wiadomość z kanału doorStatus
    doorStatus.value = 'Oczekiwanie danych...';
  };

  ws.onclose = () => {
    if (doorStatus.value !== 'Drzwi otwarte' && doorStatus.value !== 'Drzwi zamknięte') {
      doorStatus.value = 'Brak połączenia';
    }
  };

  ws.onerror = () => {
    if (doorStatus.value !== 'Drzwi otwarte' && doorStatus.value !== 'Drzwi zamknięte') {
      doorStatus.value = 'Błąd połączenia';
    }
  };

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === 'doorStatus' && data.status) {
        doorStatus.value = data.status === 'otwarte' ? 'Drzwi otwarte' : 'Drzwi zamknięte';
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
.card__icon {
  display: flex;
  justify-content: center;
  align-items: center;
  margin-top: 16px;
}

.door-icon {
  width: 48px;
  height: 48px;
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