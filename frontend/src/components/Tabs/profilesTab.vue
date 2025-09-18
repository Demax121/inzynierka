
<script setup>


import CreateProfileCard_component from '../createProfileCard_component.vue'
import ChooseProfileCard_component from '../chooseProfileCard_component.vue'


</script>

<template>
    <div class="main-container">
<create-profile-card_component />
<choose-profile-card_component />
    </div>
</template>



<style lang="scss" scoped>

.main-container {
  display: flex;
  flex-direction: row;
  align-items: center;
  padding: 1rem;
  gap: 1.5rem; /* Odstęp między kartami */
}

</style>