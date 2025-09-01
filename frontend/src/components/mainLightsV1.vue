<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title card__title--lights">Sterowanie głównym oświetleniem</h2>
        </div>
        <div class="card__body card__body--slider">
            <div class="card__content card__content--slider">
                <!-- Rounded switch -->
                <label class="switch">
                  <input 
                    type="checkbox" 
                    :checked="isLightOn" 
                    @change="toggleLights"
                  >
                  <span class="slider round"></span>
                </label>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useLinkStore } from '@/stores/linkStore'

const linkStore = useLinkStore()

// Stan świateł (zgodny z ESP32: "ON"/"OFF")
const lightStatus = ref("OFF")

// Computed property dla checkbox
const isLightOn = computed(() => lightStatus.value === "ON")

// WebSocket connection
let ws = null

// Function to fetch initial light status via HTTP
const fetchInitialStatus = async () => {
  try {
    const response = await fetch(linkStore.getPhpApiUrl('mainLightsStatus.php'))
    const data = await response.json()
    if (data.lightStatus) {
      lightStatus.value = data.lightStatus
      console.log(`Initial light status fetched: ${data.lightStatus}`)
    }
  } catch (error) {
    console.error('Failed to fetch initial light status:', error)
  }
}

// Funkcja do przełączania świateł
const toggleLights = () => {
  const newStatus = lightStatus.value === "ON" ? "OFF" : "ON"
  
  // Najpierw wyślij przez WebSocket
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({
      channel: "mainLights",
      lightStatus: newStatus
    }))
    console.log(`Lights toggle command sent via WebSocket: ${newStatus}`)
  }
  
  // Ustaw lokalny stan dla natychmiastowego feedback UI
  lightStatus.value = newStatus
}

// Inicjalizacja WebSocket
const initWebSocket = () => {
  try {
    ws = new WebSocket('ws://192.168.1.4:8886')
    
    ws.onopen = () => {
      console.log('WebSocket connected to lights server')
      // Fetch initial status when WebSocket connects
      fetchInitialStatus()
    }
    
    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data)
        
        // Obsługa wiadomości z kanału mainLights
        if (data.channel === 'mainLights' && typeof data.lightStatus === 'string') {
          const receivedStatus = data.lightStatus
          if ((receivedStatus === "ON" || receivedStatus === "OFF") && lightStatus.value !== receivedStatus) {
            lightStatus.value = receivedStatus
            console.log(`Lights state updated from external source: ${receivedStatus}`)
          }
        }
      } catch (error) {
        console.error('Error parsing WebSocket message:', error)
      }
    }
    
    ws.onclose = () => {
      console.log('WebSocket connection closed')
      // Próba ponownego połączenia po 5 sekundach
      setTimeout(initWebSocket, 5000)
    }
    
    ws.onerror = (error) => {
      console.error('WebSocket error:', error)
    }
  } catch (error) {
    console.error('Failed to initialize WebSocket:', error)
    // Próba ponownego połączenia po 5 sekundach
    setTimeout(initWebSocket, 5000)
  }
}

// Lifecycle hooks
onMounted(() => {
  initWebSocket()
})

onUnmounted(() => {
  if (ws) {
    ws.close()
  }
})
</script>

<style lang="scss" scoped>

$slider-switch-size: 3.5rem;

.card__body--slider {
  display: grid;
  justify-content: center;
  align-items: center;
}

.card__content--slider {
  height: fit-content;
}

/* The switch - the box around the slider */
.switch {
  position: relative;
  display: inline-block;
  width: 11rem;
  height: 4rem;
}

/* Hide default HTML checkbox */
.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

/* The slider */
.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #222;
  border-radius: 34px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 1.4rem;
  font-weight: bold;
  color: white;
  transition: background-color .4s, box-shadow .4s;
  overflow: hidden;
}

/* Domyślny tekst OFF */
.slider::after {
  content: "OFF";
  opacity: 1;
  transition: opacity .3s ease-in-out;
  position: relative;
  z-index: 2;
}

/* Wewnętrzne kółko */
.slider:before {
  position: absolute;
  content: "";
  height: $slider-switch-size;
  width: $slider-switch-size;
  left: 4px;
  bottom: 4px;
  background-color: white;
  border-radius: 50%;
  transition: .4s;
  z-index: 1;
}

/* Efekt glow przy ON */
input:checked + .slider {
  box-shadow: 0 0 25px 6px #d40fe6; /* glow */
}

/* Zmiana tekstu ON/OFF z fade */
input:checked + .slider::after {
  content: "ON";
  opacity: 1;
  animation: fadeText .3s ease-in-out;
}

/* Gdy zaznaczony przesuwamy kółko w prawo */
input:checked + .slider:before {
  transform: translateX(7rem);
}

/* Animacja fade dla tekstu */
@keyframes fadeText {
  0% { opacity: 0; }
  50% { opacity: 0.5; }
  100% { opacity: 1; }
}




</style>