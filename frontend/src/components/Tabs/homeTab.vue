<script setup>

import Blinds from '@/components/blinds.vue'
import Doors from '@/components/doors.vue'
import MainLights from '@/components/mainLights.vue';
import RoomStats from '@/components/roomStats.vue';
import LEDs from '@/components/wled.vue';
import AC from '@/components/klimatyzacja.vue';


</script>





<template>
    <div>
<div class="main-container">
            <Doors />
            <RoomStats />
            <Blinds />
            <MainLights />
            <LEDs />
            <AC />

        </div>
    </div>
</template>


<style lang="scss" scoped>



</style>