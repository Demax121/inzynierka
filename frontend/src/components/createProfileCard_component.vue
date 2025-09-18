<template>
  <div class="card card--big">
    <div class="card__header">
      <h2 class="card__title">Konfigurator Profili</h2>
    </div>
    <div class="card__body">
      <div class="card__content">
        <form @submit.prevent="saveProfile" class="profile-form">
          <div class="form-group">
            <label for="profileName">Nazwa profilu:</label>
            <input type="text" id="profileName" v-model="profileName" class="form-control" placeholder="Wpisz nazwę profilu" required>
          </div>
          
          <div class="form-group">
            <label for="wledPreset">Preset WLED:</label>
            <div class="preset-select-container">
              <div v-if="loading && !presetsLoaded" class="loading-indicator">
                Ładowanie presetów WLED...
              </div>
              
              <select v-if="presets.length > 0 || presetsLoaded" v-model="profileSettings.wledPreset" id="wledPreset" class="form-control preset-select" required>
                <option disabled value="">Wybierz preset WLED</option>
                <!-- Special options -->
                <option value="off">Wyłączony</option>
                <option value="ambilight">Ambilight</option>
                <!-- Regular presets from WLED -->
                <option v-for="preset in presets" :key="preset.id" :value="preset.id">
                  {{ preset.name }}
                </option>
              </select>
            </div>
          </div>

          <!-- Możesz dodać tutaj więcej opcji konfiguracyjnych dla profilu -->
          
          <!-- Hidden field to store the JSON data for form submission -->
          <input type="hidden" name="profileJSON" :value="JSON.stringify(buildProfileJSON())" />
          
          <div class="button-group">
            <button type="submit" class="btn" :disabled="!isFormValid">Zapisz profil</button>
          </div>
        </form>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, watch } from 'vue';
import { useLinkStore } from '@/stores/linkStore';

// Use the linkStore for WLED functionality
const linkStore = useLinkStore();

// Get loading and presets state for template usage
const loading = computed(() => linkStore.wledPresetsLoading);
const presetsLoaded = computed(() => linkStore.wledPresetsLoaded);
const presets = computed(() => linkStore.wledPresets);

// Profile data
const profileName = ref('');
const profileSettings = reactive({
  wledPreset: '',
  // Add other settings for the profile here
});

// Validate form
const isFormValid = computed(() => {
  return profileName.value.trim().length > 0 && profileSettings.wledPreset !== '';
});

// Build the complete profile JSON structure
function buildProfileJSON() {
  // Base profile JSON
  const profileJSON = {
    name: profileName.value,
    WLED: {
      // Default values that will be overridden based on selection
      on: true,
      lor: 2,
      ps: null
    }
  };
  
  // Configure WLED settings based on the selected option
  if (profileSettings.wledPreset === 'off') {
    // If "Wyłączone" is selected
    profileJSON.WLED.on = false;
    // lor and ps values aren't relevant when off
    delete profileJSON.WLED.lor;
    delete profileJSON.WLED.ps;
  } else if (profileSettings.wledPreset === 'ambilight') {
    // If "Ambilight" is selected
    profileJSON.WLED.on = true;
    profileJSON.WLED.lor = 0; // Ambilight mode
    // ps value isn't used in Ambilight mode
    delete profileJSON.WLED.ps;
  } else {
    // If a regular preset is selected
    profileJSON.WLED.on = true;
    profileJSON.WLED.lor = 2; // Regular WLED preset mode
    profileJSON.WLED.ps = parseInt(profileSettings.wledPreset); // Use the preset ID
  }
  
  return profileJSON;
}

// Save profile function
function saveProfile() {
  if (!isFormValid.value) return;
  
  // Build the profile JSON
  const profileData = buildProfileJSON();
  
  // Display the JSON in the console (formatted for readability)
  console.log('Profile JSON:', JSON.stringify(profileData, null, 2));
  
  // Here you would save the profile to your backend or local storage
  // This could be implemented later with a form submission to a PHP endpoint
  
  // Reset form after save
  profileName.value = '';
  profileSettings.wledPreset = '';
}

onMounted(() => {
  // Fetch presets when component mounts
  linkStore.fetchWledPresets();
});

</script>

<style lang="scss" scoped>
.form-group {
  margin-bottom: 1rem;
  
  label {
    display: block;
    margin-bottom: 0.5rem;
    font-weight: 500;
  }
}

.form-control {
  width: 100%;
  padding: 0.5rem;
  border-radius: 0.25rem;
  border: 1px solid #ccc;
  background-color: var(--color-background);
  color: var(--color-text);
}

.preset-select-container {
  margin-top: 0.25rem;
}

.loading-indicator {
  font-style: italic;
  color: var(--color-text-secondary, #777);
  margin-bottom: 0.5rem;
}

.button-group {
  margin-top: 1.5rem;
  display: flex;
  justify-content: flex-end;
}
</style>