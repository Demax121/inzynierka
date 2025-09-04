<script setup>
import { ref } from 'vue'
import Navbar from '@/components/Navbar_component.vue'
import HeroSection from '@/components/heroSection_component.vue'
import HomeTab from './components/Tabs/homeTab.vue'
import ProfilesTab from './components/Tabs/profilesTab.vue'
import WeatherTab from './components/Tabs/weatherTab.vue'
import WledTab from './components/Tabs/wledTab.vue'

const currentTab = ref('HomeTab');
const Tabs = { HomeTab, WeatherTab, WledTab, ProfilesTab };



</script>

<template>
    <div class="app">
        <HeroSection />
        <Navbar @changeTab="currentTab = $event" />

        <KeepAlive>
        <component :is="Tabs[currentTab]" />
        </KeepAlive>
    </div>
</template>

<style lang="scss">
@use "@/scss/colors" as *;

.app {
    min-height: 100vh;
}

</style>
