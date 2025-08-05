<script setup>
import Blinds from '@/components/blinds.vue'
import Doors from '@/components/doors.vue'
import MainLights from './components/mainLights.vue';
import RoomStats from './components/roomStats.vue';
import LEDs from './components/wled.vue';
import ProfileConfiguration from './components/profileConfiguration.vue';
import ChooseProfile from './components/chooseProfile.vue';

</script>

<template>
<div class="main-container">
    <Blinds/>
    <MainLights/>
    <LEDs/>
    <Doors/>
    <RoomStats/>
    <ProfileConfiguration/>
    <ChooseProfile/>
    
</div>

</template>

<style lang = "scss">
</style>
