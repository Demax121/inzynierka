<script setup>
import Blinds from '@/components/blinds.vue'
import Doors from '@/components/doors.vue'
import MainLights from './components/mainLights.vue';
import RoomStats from './components/roomStats.vue';

</script>

<template>
<div class="main-container">
    <Blinds/>
    <MainLights/>
    <Doors/>
    <RoomStats/>
    
</div>

</template>

<style lang = "scss">
</style>
