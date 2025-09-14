<script setup>
import { ref } from 'vue';
import Blinds from '@/components/blindsCard_component.vue'
import Doors from '@/components/doorsCard_component.vue'
import main_lights from '@/components/światłaCard_component.vue';
import room_stats from '@/components/roomStatsCard_component.vue';
import LEDs from '@/components/wledCard_component.vue';
import AC from '@/components/klimaCard_component.vue';
import lux_sensorCard_component from '../luxSensorCard_component.vue';
import BlindsAutomateCard_component from '../blindsAutomateCard_component.vue'

// Create a reference to the Blinds component and BlindsAutomateCard component
const blindsCardRef = ref(null);
const blindsAutomateCardRef = ref(null);

// Methods to control blinds - these will be passed to BlindsAutomateCard
const controlOpenBlinds = () => {
  if (blindsCardRef.value) {
    blindsCardRef.value.openBlinds();
  }
};

const controlCloseBlinds = () => {
  if (blindsCardRef.value) {
    blindsCardRef.value.closeBlinds();
  }
};

// Handler for manual control events from blindsCard
const handleManualControl = () => {
  if (blindsAutomateCardRef.value) {
    blindsAutomateCardRef.value.disableAutomation();
  }
};
</script>

<template>
  <div class="main-container">
    <Doors />
    <room_stats />
    <Blinds ref="blindsCardRef" @manual-control="handleManualControl" />
    <BlindsAutomateCard_component 
      ref="blindsAutomateCardRef"
      :openBlindsHandler="controlOpenBlinds"
      :closeBlindsHandler="controlCloseBlinds"
    />
    <main_lights />
    <LEDs />
    <AC />
    <lux_sensorCard_component />
  </div>
</template>

<style lang="scss" scoped>
/* Adjust if any specific overrides needed later */
</style>