<template>
  <div class="card card--big">
    <div class="card__header">
      <h2 class="card__title">Konfigurator Profili</h2>
    </div>
    <div class="card__body">
      <div class="card__content">
        <form @submit.prevent="saveProfile" class="profile-form">
          <div class="form-group">
            <label for="profileName"  class="group-title">Nazwa profilu:</label>
            <input type="text" id="profileName" v-model="profileName" class="form-control"
              placeholder="Wpisz nazwę profilu" required>
          </div>

          <div class="form-group">
            <label for="wledPreset" class="group-title">Preset WLED:</label>
            <div class="preset-select-container">
              <div v-if="loading && !presetsLoaded" class="loading-indicator">
                Ładowanie presetów WLED...
              </div>

              <select v-if="presets.length > 0 || presetsLoaded" v-model="profileSettings.wledPreset" id="wledPreset"
                class="form-control preset-select" required>
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

          <div class="form-group">
            <label for="mainLights"  class="group-title">Oświetlenie:</label>
            <div class="lights-control">
              <label class="switch">
                <input type="checkbox" v-model="profileSettings.lightsOn">
                <span class="slider round"></span>
              </label>
            </div>
          </div>

          <div class="form-group">
            <label for="acTemperature" class="group-title">Klimatyzacja:</label>
            <div class="ac-control">
              <input 
                type="number" 
                id="acTemperature" 
                v-model.number="profileSettings.acTemperature" 
                min="16" 
                max="30" 
                step="1" 
                placeholder="Temperatura (16-30°C)" 
                class="form-control temperature-input"
                required
              >
              <div class="form-help-text">Ustaw temperaturę klimatyzacji dla tego profilu</div>
            </div>
          </div>

          <div class="form-group">
            <label class="group-title">Rolety / Zasłony:</label>
            <div class="blinds-control">
              <div class="radio-group">
                <label class="radio-label">
                  <input type="radio" v-model="profileSettings.blindsMode" value="open" name="blinds-mode">
                  <span>Otwarte</span>
                </label>
                <label class="radio-label">
                  <input type="radio" v-model="profileSettings.blindsMode" value="close" name="blinds-mode">
                  <span>Zamknięte</span>
                </label>
                <label class="radio-label">
                  <input type="radio" v-model="profileSettings.blindsMode" value="auto" name="blinds-mode">
                  <span>Automatyzacja</span>
                </label>
              </div>
              
              <!-- Conditional lux settings when auto mode is selected -->
              <div v-if="profileSettings.blindsMode === 'auto'" class="lux-settings">
                <div class="lux-config-form">
                  <div class="input-group">
                    <label for="minLux">Min lux:</label>
                    <input id="minLux" v-model.number="profileSettings.minLux" type="number" class="lux-input" required>
                    <span class="lux-explanation">Powyżej tej wartości zasłony zostaną podniesione</span>
                  </div>
                  <div class="input-group">
                    <label for="maxLux">Max lux:</label>
                    <input id="maxLux" v-model.number="profileSettings.maxLux" type="number" class="lux-input" required>
                    <span class="lux-explanation">Powyżej tej wartości zasłony zostaną opuszczone</span>
                  </div>
                </div>
                <div class="form-help-text">Ustaw granice jasności dla automatyki rolet</div>
              </div>
            </div>
          </div>

          <!-- Hidden field to store the JSON data for form submission -->
          <input type="hidden" name="profileJSON" :value="JSON.stringify(buildProfileJSON())" />

          <div class="button-group">
            <button type="submit" class="btn">Zapisz profil</button>
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
  lightsOn: false, // Default state for main lights
  acTemperature: null, // Temperature setting for AC
  blindsMode: 'open', // Default to 'open', options: 'open', 'close', 'auto'
  minLux: 5000, // Default min lux value
  maxLux: 20000, // Default max lux value
  // Add other settings for the profile here
});

// No custom validation, using HTML required attributes instead

// Build the complete profile JSON structure
function buildProfileJSON() {
  // Base profile JSON (excluding the name, which will be saved separately)
  const profileJSON = {
    WLED: {
      // Default values that will be overridden based on selection
      on: true,
      lor: 2,
      ps: null
    },
    lights: {
      channel: "main_lights",
      payload: {
        state: profileSettings.lightsOn
      }
    }
  };
  
  // Add AC settings if temperature is set
  if (profileSettings.acTemperature !== null && profileSettings.acTemperature !== '') {
    profileJSON.AC = {
      channel: "air_conditioning",
      payload: {
        requestedTemp: profileSettings.acTemperature
      }
    };
  }

  // Add blinds settings based on selected mode
  if (profileSettings.blindsMode === 'open' || profileSettings.blindsMode === 'close') {
    profileJSON.blinds = {
      state: profileSettings.blindsMode // 'open' or 'close'
    };
  } else if (profileSettings.blindsMode === 'auto') {
    // For auto mode, include min/max lux settings
    profileJSON.blinds = {
      minLux: profileSettings.minLux,
      maxLux: profileSettings.maxLux,
      automate: true
    };
  }

  // Configure WLED settings based on the selected option
  if (profileSettings.wledPreset === 'off') {
    // If "Wyłączone" is selected
    profileJSON.WLED.on = false;
    profileJSON.WLED.preset_name = "Wyłączony";
    // lor and ps values aren't relevant when off
    delete profileJSON.WLED.lor;
    delete profileJSON.WLED.ps;
  } else if (profileSettings.wledPreset === 'ambilight') {
    // If "Ambilight" is selected
    profileJSON.WLED.on = true;
    profileJSON.WLED.lor = 0; // Ambilight mode
    profileJSON.WLED.preset_name = "Ambilight";
    // ps value isn't used in Ambilight mode
    delete profileJSON.WLED.ps;
  } else {
    // If a regular preset is selected
    profileJSON.WLED.on = true;
    profileJSON.WLED.lor = 2; // Regular WLED preset mode
    profileJSON.WLED.ps = parseInt(profileSettings.wledPreset); // Use the preset ID
    
    // Find the preset name from the preset ID
    const presetObj = presets.value.find(p => p.id.toString() === profileSettings.wledPreset.toString());
    if (presetObj) {
      profileJSON.WLED.preset_name = presetObj.name;
    } else {
      profileJSON.WLED.preset_name = `Preset ${profileSettings.wledPreset}`;
    }
  }

  return profileJSON;
}

// Define props to receive shared profile names from parent
const props = defineProps({
  sharedProfileNames: {
    type: Object, // Set object
    default: () => new Set()
  }
});

// Define emits to send updates to parent
const emit = defineEmits(['profile-created']);

// Save profile function
function saveProfile() {
  // Validate profile name
  if (!profileName.value.trim()) {
    alert('Proszę podać nazwę profilu');
    return;
  }
  
  // Check if profile name already exists in the shared Set
  if (props.sharedProfileNames.has(profileName.value)) {
    alert('Profil o tej nazwie już istnieje. Wybierz inną nazwę.');
    return;
  }
  
  // Build the profile JSON
  const profileData = buildProfileJSON();



  // Send profile to backend
  const saveData = {
    profile_name: profileName.value,
    profile_json: profileData
  };
  
  // Show saving indicator
  const saveBtn = document.querySelector('.button-group button');
  const originalText = saveBtn.innerText;
  saveBtn.innerText = 'Zapisywanie...';
  saveBtn.disabled = true;
  
  // Send to PHP endpoint
  fetch(linkStore.getPhpApiUrl('createProfile.php'), {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(saveData)
  })
  .then(response => response.json())
  .then(data => {
    if (data.success) {
      alert('Profil został zapisany!');
      
      // Update the shared profile names
      const updatedNames = new Set(props.sharedProfileNames);
      updatedNames.add(profileName.value);
      emit('profile-created', updatedNames);
      
      // Reset form after successful save
      profileName.value = '';
      profileSettings.wledPreset = '';
      profileSettings.lightsOn = false; // Reset lightsOn state
      profileSettings.acTemperature = null; // Reset temperature setting
      profileSettings.blindsMode = 'open'; // Reset blinds mode to default
      profileSettings.minLux = 5000; // Reset min lux
      profileSettings.maxLux = 20000; // Reset max lux
    } else {
      alert('Błąd: ' + (data.error || 'Nie udało się zapisać profilu'));
    }
  })
  .catch(error => {
    alert('Błąd połączenia: ' + error.message);
  })
  .finally(() => {
    // Restore button state
    saveBtn.innerText = originalText;
    saveBtn.disabled = false;
  });
}

// Using HTML required attributes for validation instead

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

/* Layout for lights control */
.lights-control {
  display: flex;
  justify-content: left;
  align-items: center;
  margin-top: 0.5rem;
}

/* Layout for AC control */
.ac-control {
  display: flex;
  flex-direction: column;
  margin-top: 0.5rem;
}

.temperature-input {
  width: 100%;
  max-width: 200px;
}

/* Layout for blinds control */
.blinds-control {
  display: flex;
  flex-direction: column;
  margin-top: 0.5rem;
}

.radio-group {
  display: flex;
  flex-direction: row;
  gap: 1.5rem;
  margin-bottom: 1rem;
}

.radio-label {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  cursor: pointer;
  user-select: none;
}

.lux-settings {
  margin-top: 0.5rem;
  padding-top: 0.5rem;
  border-top: 1px dashed #ccc;
}

.lux-config-form {
  margin-bottom: 0.5rem;
}

.input-group {
  display: flex;
  align-items: center;
  margin-bottom: 0.75rem;
  gap: 1rem;
  justify-content: flex-start;
  flex-wrap: wrap;
}

.lux-input {
  padding: 6px 10px;
  font-size: 11pt;
  width: 120px;
  border: 2px solid #ccc;
  border-radius: 4px;
  outline: none;
  transition: border-color 0.3s;
}

.lux-explanation {
  font-size: 0.85rem;
  color: var(--color-text-secondary, #777);
  margin-left: 0.5rem;
  flex: 1;
}

.form-help-text {
  font-size: 0.85rem;
  color: var(--color-text-secondary, #777);
  margin-top: 0.25rem;
  margin-left: 0.25rem;
}

.group-title{
  font-size: 13pt;
  padding-left: 0.75rem;
}


</style>