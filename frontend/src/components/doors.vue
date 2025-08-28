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

const doorStatus = ref('Łączenie...');
let ws;

// Function to fetch initial door status via HTTP
const fetchInitialStatus = async () => {
  try {
    const response = await fetch(linkStore.getPhpApiUrl('doorStatus.php'));
    const data = await response.json();
    if (data.status) {
      doorStatus.value = data.status === "otwarte" ? "Drzwi otwarte" : "Drzwi zamknięte";
    }
  } catch (error) {
    console.error('Failed to fetch initial door status:', error);
    doorStatus.value = 'Błąd pobierania danych';
  }
};

onMounted(() => {
  ws = new WebSocket('ws://192.168.1.4:8886'); // Adres serwera WebSocket

  ws.onopen = () => {
    doorStatus.value = 'Pobieranie danych...';
    fetchInitialStatus();
  };

  ws.onclose = () => {
    doorStatus.value = 'Brak połączenia';
  };

  ws.onerror = (error) => {
    console.error('Door WebSocket error:', error);
    doorStatus.value = 'Błąd połączenia';
  };

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.channel === "doorStatus") {
        doorStatus.value = data.status === "otwarte" ? "Drzwi otwarte" : "Drzwi zamknięte";
      }
    } catch (e) {
      console.error('Door JSON parse error:', e);
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