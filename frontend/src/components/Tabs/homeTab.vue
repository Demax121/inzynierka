<script setup>
import { ref } from 'vue';
import Blinds from '@/components/blindsCard_component.vue'
import Doors from '@/components/doorsCard_component.vue'
import main_lights from '@/components/mainLightsCard_component.vue';
import room_stats from '@/components/roomStatsCard_component.vue';
import LEDs from '@/components/wledCard_component.vue';
import AC from '@/components/airConditioningCard_component.vue';
import lux_sensorCard_component from '../luxSensorCard_component.vue';
import BlindsAutomateCard_component from '../blindsAutomateCard_component.vue'
</script>

<template>
  <div class="main-container">
    <Doors />
    <room_stats />
    <Blinds />
    <BlindsAutomateCard_component />
    <main_lights />
    <LEDs />
    <AC />
    <lux_sensorCard_component />
  </div>
</template>

<style lang="scss" scoped>

</style>