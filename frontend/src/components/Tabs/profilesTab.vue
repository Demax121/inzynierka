
<script setup>
import { ref, reactive } from 'vue';
import CreateProfileCard_component from '../createProfileCard_component.vue';
import ChooseProfileCard_component from '../chooseProfileCard_component.vue';

// Create a reactive Set to store profile names
// This will be shared between both child components
const sharedProfileNames = reactive(new Set());

// Function to update the shared profile names
function updateProfileNames(profileNames) {
  // Clear the current set and add new names
  sharedProfileNames.clear();
  profileNames.forEach(name => {
    sharedProfileNames.add(name);
  });
}
</script>

<template>
  <div class="main-container">
    <create-profile-card_component 
      :shared-profile-names="sharedProfileNames" 
      @profile-created="updateProfileNames" 
    />
    <choose-profile-card_component 
      :shared-profile-names="sharedProfileNames" 
      @update-profiles="updateProfileNames" 
    />
  </div>
</template>

<style lang="scss" scoped>
.main-container {
  display: flex;
  flex-direction: row;
  align-items: stretch; /* Changed to stretch to make cards equal height */
  padding: 1rem;
  gap: 1.5rem; /* Space between cards */
  width: 100%;
  justify-content: center; /* Center the cards horizontally */
}

/* Make cards the same width */
:deep(.card) {
  flex: 1; /* Each card takes equal space */
  max-width: 48%; /* Prevent cards from getting too wide */
  min-width: 350px; /* Ensure minimum width for readability */
}

/* Make cards responsive but only on very small screens */
@media (max-width: 900px) {
  .main-container {
    flex-direction: column;
    align-items: center;
  }
  
  :deep(.card) {
    max-width: 100%;
    width: 100%;
  }
}
</style>