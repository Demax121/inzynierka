<script setup>
import { ref } from 'vue'
import Navbar from '@/components/Navbar.vue'
import HeroSection from '@/components/heroSection.vue'
import HomeTab from './components/Tabs/homeTab.vue'


</script>

<template>
    <div class="app">
        <HeroSection />
        <Navbar />
        <HomeTab />
    </div>
</template>

<style lang="scss">
@use "@/scss/colors" as *;

.app {
    min-height: 100vh;
}

</style>
