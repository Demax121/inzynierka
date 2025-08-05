<script setup>
import Blinds from '@/components/blinds.vue'
import Doors from '@/components/doors.vue'
import MainLights from './components/mainLights.vue';
import RoomStats from './components/roomStats.vue';
import LEDs from './components/wled.vue';
import ProfileConfiguration from './components/profileConfiguration.vue';
import ChooseProfile from './components/chooseProfile.vue';
import Weather from './components/weather.vue';
import AC from './components/klimatyzacja.vue';

</script>

<template>
<div class="main-container">
    <Blinds/>
    <MainLights/>
    <LEDs/>
    <AC/>
    <Doors/>
    <RoomStats/>
    <ProfileConfiguration/>
    <ChooseProfile/>


</div>
    <Weather/>
</template>

<style lang = "scss">
</style>
