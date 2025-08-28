<script setup>

import Blinds from '@/components/blinds.vue'
import Doors from '@/components/doors.vue'
import MainLights from '@/components/mainLights.vue';
import RoomStats from '@/components/roomStats.vue';
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