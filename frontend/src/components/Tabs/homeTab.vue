<script setup>

import Blinds from '@/components/blinds.vue'
import Doors from '@/components/doorSensorV2.vue'
import MainLights from '@/components/mainLightsV2.vue';
import RoomStats from '@/components/roomStatsV2.vue';
import LEDs from '@/components/wled.vue';
import AC from '@/components/klimatyzacja.vue';


</script>

<template>
  <div class="main-container">
    <Doors />
    <RoomStats />
    <Blinds />
    <MainLights />
    <LEDs />
    <AC />
  </div>
</template>

<style lang="scss" scoped>
/* Adjust if any specific overrides needed later */
</style>