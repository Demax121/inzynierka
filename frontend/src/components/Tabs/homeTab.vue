<script setup>

import Blinds from '@/components/blindsCard_component.vue'
import Doors from '@/components/doorsCard_component.vue'
import MainLights from '@/components/światłaCard_component.vue';
import RoomStats from '@/components/roomStatsCard_component.vue';
import LEDs from '@/components/wledCard_component.vue';
import AC from '@/components/klimaCard_component.vue';
import LuxSensorCard_component from '../luxSensorCard_component.vue'


</script>

<template>
  <div class="main-container">
    <Doors />
    <RoomStats />
    <Blinds />
    <MainLights />
    <LEDs />
    <AC />
    <LuxSensorCard_component />
  </div>
</template>

<style lang="scss" scoped>
/* Adjust if any specific overrides needed later */
</style>