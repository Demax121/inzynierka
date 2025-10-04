<template>
  <div class="card card--big">
    <div class="card__header">
      <h2 class="card__title">Profile generator</h2>
    </div>
    <div class="card__body">
      <div class="card__content">
        <form @submit.prevent="saveProfile" class="profile-form">
          <div class="form-group">
            <label for="profileName"  class="group-title">Profile Name:</label>
            <input type="text" id="profileName" v-model="profileName" class="form-control"
              placeholder="Wpisz nazwę profilu" required>
          </div>

          <div class="form-group">
            <label for="wledPreset" class="group-title">WLED Preset:</label>
            <div class="preset-select-container">
              <div v-if="loading && !presetsLoaded" class="loading-indicator">
                Loading presets...
              </div>

              <select v-if="presets.length > 0 || presetsLoaded" v-model="profileSettings.wledPreset" id="wledPreset"
                class="form-control preset-select" required>
                <option disabled value="">Choose WLED Preset</option>
                <!-- Special options -->
                <option value="off">OFF</option>
                <option value="ambilight">Ambilight</option>
                <!-- Regular presets from WLED -->
                <option v-for="preset in presets" :key="preset.id" :value="preset.id">
                  {{ preset.name }}
                </option>
              </select>
            </div>
          </div>

          <div class="form-group">
            <label for="mainLights"  class="group-title">Main Lights:</label>
            <div class="lights-control">
              <label class="switch">
                <input type="checkbox" v-model="profileSettings.lightsOn">
                <span class="slider round"></span>
              </label>
            </div>
          </div>

          <div class="form-group">
            <label for="acTemperature" class="group-title">AC Control:</label>
            <div class="ac-control">
              <input 
                type="double" 
                id="acTemperature" 
                v-model.number="profileSettings.acTemperature" 
                min="16" 
                max="30" 
                step="1" 
                placeholder="Set temperature (16-30°C)" 
                class="form-control temperature-input"
                required
              >
              
            </div>
          </div>

          <div class="form-group">
            <label class="group-title">Window Blinds:</label>
            <div class="blinds-control">
              <div class="radio-group">
                <label class="radio-label">
                  <input type="radio" v-model="profileSettings.blindsMode" value="open" name="blinds-mode">
                  <span>Open</span>
                </label>
                <label class="radio-label">
                  <input type="radio" v-model="profileSettings.blindsMode" value="close" name="blinds-mode">
                  <span>Closed</span>
                </label>
                <label class="radio-label">
                  <input type="radio" v-model="profileSettings.blindsMode" value="auto" name="blinds-mode">
                  <span>Automate</span>
                </label>
              </div>
              
              <!-- Conditional lux settings when auto mode is selected -->
              <div v-if="profileSettings.blindsMode === 'auto'" class="lux-settings">
                <div class="form-help-text">Set treshold limits for this profile below:</div>
                <div class="lux-config-form">
                  <div class="input-group">
                    
                    <label for="minLux">Min lux:</label>
                    <input id="minLux" v-model.number="profileSettings.minLux" type="number" class="lux-input" required>
                    <span class="lux-explanation">Above this treshold blinds will open</span>
                  </div>
                  <div class="input-group">
                    <label for="maxLux">Max lux:</label>
                    <input id="maxLux" v-model.number="profileSettings.maxLux" type="number" class="lux-input" required>
                    <span class="lux-explanation">Above this treshold blinds will be closed</span>
                  </div>
                </div>
                
              </div>
            </div>
          </div>

          <!-- Hidden field to store the JSON data for form submission -->
          <input type="hidden" name="profileJSON" :value="JSON.stringify(buildProfileJSON())" />

          <div class="button-group">
            <button type="submit" class="btn">Save profile</button>
          </div>
        </form>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted } from 'vue';
import { useLinkStore } from '@/stores/linkStore';

const linkStore = useLinkStore();

const loading = computed(() => linkStore.wledPresetsLoading);
const presetsLoaded = computed(() => linkStore.wledPresetsLoaded);
const presets = computed(() => linkStore.wledPresets);

const profileName = ref('');
const profileSettings = reactive({
  wledPreset: '',
  lightsOn: false,
  acTemperature: null,
  blindsMode: 'open',
  minLux: 5000,
  maxLux: 20000,
});

// Security: same regex as backend (keep in sync)
const NAME_REGEX = /^[A-Za-z0-9 _-]{1,64}$/;
const profileNameError = ref('');

function validateProfileName() {
  const name = profileName.value.trim();
  if (!name) {
    profileNameError.value = 'Name required';
    return false;
  }
  if (!NAME_REGEX.test(name)) {
    profileNameError.value = 'Allowed: letters, numbers, space, _ , - (max 64)';
    return false;
  }
  profileNameError.value = '';
  return true;
}

function buildProfileJSON() {
  const profileJSON = {
    WLED: { on: true, lor: 2, ps: null },
    lights: {
      channel: 'main_lights',
      payload: { state: profileSettings.lightsOn }
    }
  };
  if (profileSettings.acTemperature !== null && profileSettings.acTemperature !== '') {
    profileJSON.AC = {
      channel: 'air_conditioning',
      payload: { requestedTemp: profileSettings.acTemperature }
    };
  }
  if (profileSettings.blindsMode === 'open' || profileSettings.blindsMode === 'close') {
    profileJSON.blinds = { state: profileSettings.blindsMode };
  } else if (profileSettings.blindsMode === 'auto') {
    profileJSON.blinds = {
      minLux: profileSettings.minLux,
      maxLux: profileSettings.maxLux,
      automate: true
    };
  }
  if (profileSettings.wledPreset === 'off') {
    profileJSON.WLED.on = false;
    profileJSON.WLED.preset_name = 'OFF';
    delete profileJSON.WLED.lor;
    delete profileJSON.WLED.ps;
  } else if (profileSettings.wledPreset === 'ambilight') {
    profileJSON.WLED.on = true;
    profileJSON.WLED.lor = 0;
    profileJSON.WLED.preset_name = 'Ambilight';
    delete profileJSON.WLED.ps;
  } else if (profileSettings.wledPreset) {
    profileJSON.WLED.on = true;
    profileJSON.WLED.lor = 2;
    profileJSON.WLED.ps = parseInt(profileSettings.wledPreset);
    const presetObj = presets.value.find(p => p.id.toString() === profileSettings.wledPreset.toString());
    profileJSON.WLED.preset_name = presetObj ? presetObj.name : `Preset ${profileSettings.wledPreset}`;
  }
  return profileJSON;
}

const props = defineProps({
  sharedProfileNames: { type: Object, default: () => new Set() }
});
const emit = defineEmits(['profile-created']);

function saveProfile() {
  if (!validateProfileName()) {
    alert(profileNameError.value);
    return;
  }
  if (props.sharedProfileNames.has(profileName.value.trim())) {
    alert('Profile with this name already exists.');
    return;
  }

  const profileData = buildProfileJSON();
  const saveData = {
    profile_name: profileName.value.trim(),
    profile_json: profileData
  };

  const saveBtn = document.querySelector('.button-group button');
  const originalText = saveBtn.innerText;
  saveBtn.innerText = 'Zapisywanie...';
  saveBtn.disabled = true;

  fetch(linkStore.getPhpApiUrl('createProfile.php'), {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(saveData)
  })
  .then(r => r.json())
  .then(d => {
    if (d.success) {
      alert('Profil został zapisany!');
      const updated = new Set(props.sharedProfileNames);
      updated.add(profileName.value.trim());
      emit('profile-created', updated);
      profileName.value = '';
      profileSettings.wledPreset = '';
      profileSettings.lightsOn = false;
      profileSettings.acTemperature = null;
      profileSettings.blindsMode = 'open';
      profileSettings.minLux = 5000;
      profileSettings.maxLux = 20000;
    } else {
      alert('Błąd: ' + (d.error || 'Nie udało się zapisać profilu'));
    }
  })
  .catch(e => alert('Błąd połączenia: ' + e.message))
  .finally(() => {
    saveBtn.innerText = originalText;
    saveBtn.disabled = false;
  });
}

onMounted(() => {
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
  font-size: .9rem;
  color: var(--color-text-secondary, #777);
  margin: .25rem;
  text-decoration: underline;
  font-weight: 700;
}

.group-title{
  font-size: 13pt;
  padding-left: 0.75rem;
  
}


</style>