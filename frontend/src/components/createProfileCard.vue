<template>
  <div class="card card--big">
    <div class="card__header">
      <h2 class="card__title">Konfigurator Profili</h2>
    </div>
    <div class="card__body">
      <div class="card__content">
        <form class="profile-form" @submit.prevent="saveProfile">
          <!-- Profile Name -->
          <div class="form-group">
            <label for="profileName" class="form-label">Nazwa profilu:</label>
            <input 
              type="text" 
              id="profileName" 
              class="form-input"
              placeholder="Wprowadź nazwę profilu"
              v-model="profile.name"
              required
            />
          </div>

          <!-- Air Conditioning -->
          <div class="form-section">
            <h3 class="form-section__title">Klimatyzacja</h3>
            
            <div class="form-group form-group--row">
              <label class="form-switch">
                <input type="checkbox" v-model="profile.AC.payload.status">
                <span class="form-switch__slider"></span>
                <span class="form-switch__label">{{ profile.AC.payload.status ? 'Włączona' : 'Wyłączona' }}</span>
              </label>
            </div>

            <div class="form-group" v-if="profile.AC.payload.status">
              <label for="acTemp" class="form-label">Temperatura docelowa (°C):</label>
              <div class="form-slider-container">
                <input 
                  type="range" 
                  id="acTemp" 
                  class="form-slider"
                  v-model.number="profile.AC.payload.targetTemp"
                  min="16"
                  max="30"
                  step="0.5"
                />
                <span class="form-slider__value">{{ profile.AC.payload.targetTemp }}°C</span>
              </div>
            </div>
          </div>

          <!-- Main Lights -->
          <div class="form-section">
            <h3 class="form-section__title">Główne oświetlenie</h3>
            
            <div class="form-group form-group--row">
              <label class="form-switch">
                <input type="checkbox" v-model="profile.lights.payload.state">
                <span class="form-switch__slider"></span>
                <span class="form-switch__label">{{ profile.lights.payload.state ? 'Włączone' : 'Wyłączone' }}</span>
              </label>
            </div>
          </div>

          <!-- Blinds -->
          <div class="form-section">
            <h3 class="form-section__title">Rolety</h3>
            
            <div class="form-group">
              <div class="radio-group">
                <label class="radio-item">
                  <input type="radio" v-model="blindsMode" value="open">
                  <span class="radio-label">Otwarte</span>
                </label>
                
                <label class="radio-item">
                  <input type="radio" v-model="blindsMode" value="close">
                  <span class="radio-label">Zamknięte</span>
                </label>
                
                <label class="radio-item">
                  <input type="radio" v-model="blindsMode" value="automate">
                  <span class="radio-label">Automatycznie</span>
                </label>
              </div>
            </div>

            <div class="form-group" v-if="blindsMode === 'automate'">
              <div class="form-row">
                <div class="form-col">
                  <label for="minLux" class="form-label">Minimalna jasność (lux):</label>
                  <input 
                    type="number" 
                    id="minLux" 
                    class="form-input"
                    v-model.number="profile.Blinds.min_lux"
                    min="0"
                    step="10"
                    required
                  />
                  <small class="form-hint">Poniżej tej wartości rolety zostaną otwarte</small>
                </div>
                
                <div class="form-col">
                  <label for="maxLux" class="form-label">Maksymalna jasność (lux):</label>
                  <input 
                    type="number" 
                    id="maxLux" 
                    class="form-input"
                    v-model.number="profile.Blinds.max_lux"
                    min="0"
                    step="10"
                    required
                  />
                  <small class="form-hint">Powyżej tej wartości rolety zostaną zamknięte</small>
                </div>
              </div>
            </div>
          </div>

          <!-- WLED -->
          <div class="form-section">
            <h3 class="form-section__title">Oświetlenie LED</h3>
            
            <div class="form-group">
              <label for="wledMode" class="form-label">Tryb LED:</label>
              <select 
                id="wledMode" 
                class="form-select"
                v-model="wledMode"
                required
              >
                <option value="off">Wyłączony</option>
                <option value="wled">WLED</option>
                <option value="ambilight">Ambilight</option>
              </select>
            </div>

            <template v-if="wledMode === 'wled'">
              <div class="form-group">
                <label for="wledPreset" class="form-label">Preset efektów:</label>
                <select 
                  id="wledPreset" 
                  class="form-select"
                  v-model="selectedPreset"
                >
                  <option v-for="preset in wledPresets" :key="preset.id" :value="preset.id">
                    {{ preset.name }}
                  </option>
                </select>
              </div>

              <div class="form-group">
                <label for="wledBrightness" class="form-label">Jasność:</label>
                <div class="form-slider-container">
                  <input 
                    type="range" 
                    id="wledBrightness" 
                    class="form-slider"
                    v-model.number="profile.WLED['192.168.1.25/json/state'].bri"
                    min="0"
                    max="255"
                    step="1"
                  />
                  <span class="form-slider__value">{{ Math.round((profile.WLED['192.168.1.25/json/state'].bri / 255) * 100) }}%</span>
                </div>
              </div>
            </template>
          </div>

          <!-- Submit -->
          <div class="form-actions">
            <button type="submit" class="btn btn--primary" :disabled="loading">
              {{ loading ? 'Zapisywanie...' : 'Zapisz profil' }}
            </button>
            <button type="button" class="btn btn--secondary" @click="resetForm">
              Wyczyść formularz
            </button>
          </div>

          <!-- Status message -->
          <div v-if="statusMessage" :class="['status-message', statusMessage.type]">
            {{ statusMessage.text }}
          </div>
        </form>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, watch } from 'vue';
import { useLinkStore } from '@/stores/linkStore';

const linkStore = useLinkStore();
const loading = ref(false);
const statusMessage = ref(null);

// WLED presets - these could be fetched from the API
const wledPresets = ref([
  { id: 1, name: 'Rainbow' },
  { id: 2, name: 'Colorful' },
  { id: 3, name: 'Solid Red' },
  { id: 4, name: 'Solid Blue' },
  { id: 5, name: 'Solid Green' },
  { id: 11, name: 'Police Lights' },
  { id: 16, name: 'Fire' },
]);

// Mode selections that affect the profile structure
const blindsMode = ref('open');
const wledMode = ref('off');
const selectedPreset = ref(1);

// The profile object that will be sent to the server
const profile = reactive({
  name: '',
  AC: {
    channel: 'air_conditioning',
    payload: {
      targetTemp: 22,
      status: false
    }
  },
  lights: {
    channel: 'main_lights',
    payload: {
      state: false
    }
  },
  Blinds: {
    status: 'open',
    min_lux: 100,
    max_lux: 1000,
    automate: false
  },
  WLED: {
    '192.168.1.25/json/state': {
      on: false,
      bri: 128,
      seg: {
        fx: 1
      }
    }
  }
});

// Watch for changes in mode selections and update the profile object
watch(blindsMode, (newMode) => {
  if (newMode === 'open') {
    profile.Blinds = { status: 'open' };
  } else if (newMode === 'close') {
    profile.Blinds = { status: 'close' };
  } else if (newMode === 'automate') {
    profile.Blinds = {
      automate: true,
      min_lux: 100,
      max_lux: 1000
    };
  }
});

watch(wledMode, (newMode) => {
  if (newMode === 'off') {
    profile.WLED['192.168.1.25/json/state'].on = false;
  } else if (newMode === 'wled') {
    profile.WLED['192.168.1.25/json/state'].on = true;
    profile.WLED['192.168.1.25/json/state'].seg.fx = selectedPreset.value;
  } else if (newMode === 'ambilight') {
    profile.WLED['192.168.1.25/json/state'].on = true;
    // Special FX value for Ambilight mode
    profile.WLED['192.168.1.25/json/state'].seg.fx = 0;
  }
});

watch(selectedPreset, (newPreset) => {
  if (wledMode.value === 'wled') {
    profile.WLED['192.168.1.25/json/state'].seg.fx = newPreset;
  }
});

// Fetch WLED presets from API (if possible)
const fetchWledPresets = async () => {
  try {
    const response = await fetch('http://192.168.1.25/json/presets');
    const data = await response.json();
    
    // Process the presets data into a usable format
    const presetArray = [];
    for (const key in data) {
      if (data.hasOwnProperty(key) && !isNaN(parseInt(key))) {
        presetArray.push({
          id: parseInt(key),
          name: data[key].n || `Preset ${key}`
        });
      }
    }
    
    if (presetArray.length > 0) {
      wledPresets.value = presetArray;
      selectedPreset.value = presetArray[0].id;
    }
  } catch (error) {
    console.error('Failed to fetch WLED presets:', error);
    // Keep the default presets if API fetch fails
  }
};

// Save the profile to PostgreSQL
const saveProfile = async () => {
  loading.value = true;
  statusMessage.value = null;
  
  try {
    // Prepare the final profile object based on the current mode selections
    const finalProfile = {
      name: profile.name,
      AC: profile.AC,
      lights: profile.lights,
      Blinds: profile.Blinds,
      WLED: profile.WLED
    };
    
    // Send the profile to the server
    const response = await fetch(linkStore.getPhpApiUrl('saveProfile.php'), {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(finalProfile),
    });
    
    const data = await response.json();
    
    if (data.success) {
      statusMessage.value = {
        type: 'success',
        text: 'Profil został zapisany pomyślnie!'
      };
    } else {
      throw new Error(data.message || 'Wystąpił błąd podczas zapisywania profilu.');
    }
  } catch (error) {
    console.error('Error saving profile:', error);
    statusMessage.value = {
      type: 'error',
      text: error.message || 'Wystąpił błąd podczas zapisywania profilu.'
    };
  } finally {
    loading.value = false;
  }
};

// Reset the form to default values
const resetForm = () => {
  profile.name = '';
  profile.AC.payload.targetTemp = 22;
  profile.AC.payload.status = false;
  profile.lights.payload.state = false;
  blindsMode.value = 'open';
  wledMode.value = 'off';
  selectedPreset.value = 1;
  profile.WLED['192.168.1.25/json/state'].bri = 128;
  
  statusMessage.value = null;
};

// Initialize component
onMounted(() => {
  fetchWledPresets();
});
</script>

<style lang="scss" scoped>
.profile-form {
  max-width: 800px;
  margin-bottom: 2rem;
}

.form-section {
  margin-bottom: 2rem;
  padding: 1.5rem;
  background-color: var(--bg-light, #f8f9fa);
  border-radius: 12px;
  border: 1px solid var(--border-color, #e1e5e9);

  &__title {
    margin: 0 0 1.5rem 0;
    color: var(--text-primary, #333);
    font-size: 1.2rem;
    font-weight: 600;
  }
}

.form-group {
  margin-bottom: 1.5rem;

  &:last-child {
    margin-bottom: 0;
  }
  
  &--row {
    display: flex;
    align-items: center;
  }
}

.form-row {
  display: flex;
  flex-wrap: wrap;
  margin: -0.75rem;
}

.form-col {
  flex: 1 0 calc(50% - 1.5rem);
  padding: 0.75rem;
  min-width: 200px;
}

.form-label {
  display: block;
  font-weight: 600;
  margin-bottom: 0.5rem;
  color: var(--text-primary, #333);
  font-size: 0.95rem;
}

.form-hint {
  display: block;
  margin-top: 0.25rem;
  font-size: 0.85rem;
  color: var(--text-secondary, #666);
}

.form-input,
.form-select {
  width: 100%;
  padding: 0.75rem 1rem;
  border: 2px solid var(--border-color, #e1e5e9);
  border-radius: 8px;
  font-size: 1rem;
  transition: border-color 0.2s ease, box-shadow 0.2s ease;
  background-color: var(--input-bg, #fff);

  &:focus {
    outline: none;
    border-color: var(--primary-color, #007bff);
    box-shadow: 0 0 0 3px rgba(0, 123, 255, 0.1);
  }

  &:disabled {
    background-color: var(--disabled-bg, #f8f9fa);
    color: var(--disabled-text, #6c757d);
    cursor: not-allowed;
  }
}

.form-select {
  cursor: pointer;
  appearance: none;
  background-image: url("data:image/svg+xml,%3csvg xmlns='http://www.w3.org/2000/svg' fill='none' viewBox='0 0 20 20'%3e%3cpath stroke='%236b7280' stroke-linecap='round' stroke-linejoin='round' stroke-width='1.5' d='m6 8 4 4 4-4'/%3e%3c/svg%3e");
  background-position: right 0.75rem center;
  background-repeat: no-repeat;
  background-size: 1.5em 1.5em;
  padding-right: 2.5rem;
}

.form-slider-container {
  display: flex;
  align-items: center;
  gap: 1rem;
}

.form-slider {
  flex: 1;
  height: 8px;
  border-radius: 4px;
  background: var(--bg-slider, #e1e5e9);
  outline: none;
  appearance: none;
  
  &::-webkit-slider-thumb {
    appearance: none;
    width: 18px;
    height: 18px;
    border-radius: 50%;
    background: var(--primary-color, #007bff);
    cursor: pointer;
    border: none;
    box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);
  }
  
  &::-moz-range-thumb {
    width: 18px;
    height: 18px;
    border-radius: 50%;
    background: var(--primary-color, #007bff);
    cursor: pointer;
    border: none;
    box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);
  }
  
  &__value {
    min-width: 50px;
    text-align: right;
    font-weight: 600;
    color: var(--text-primary, #333);
  }
}

.form-switch {
  position: relative;
  display: inline-flex;
  align-items: center;
  cursor: pointer;
  
  input {
    opacity: 0;
    width: 0;
    height: 0;
    position: absolute;
    
    &:checked + .form-switch__slider {
      background-color: var(--primary-color, #007bff);
      
      &:before {
        transform: translateX(22px);
      }
    }
    
    &:focus + .form-switch__slider {
      box-shadow: 0 0 0 3px rgba(0, 123, 255, 0.1);
    }
    
    &:disabled + .form-switch__slider {
      opacity: 0.5;
      cursor: not-allowed;
    }
  }
  
  &__slider {
    position: relative;
    display: inline-block;
    width: 48px;
    height: 26px;
    background-color: var(--bg-switch-off, #ccc);
    border-radius: 34px;
    transition: background-color 0.2s;
    
    &:before {
      position: absolute;
      content: "";
      height: 20px;
      width: 20px;
      left: 3px;
      bottom: 3px;
      background-color: white;
      border-radius: 50%;
      transition: transform 0.2s;
    }
  }
  
  &__label {
    margin-left: 10px;
    font-size: 0.95rem;
    font-weight: 500;
  }
}

.radio-group {
  display: flex;
  flex-wrap: wrap;
  gap: 1rem;
}

.radio-item {
  position: relative;
  cursor: pointer;
  user-select: none;
  display: flex;
  align-items: center;
  
  input[type="radio"] {
    position: absolute;
    opacity: 0;
    cursor: pointer;
    
    &:checked + .radio-label {
      background-color: var(--primary-color, #007bff);
      color: white;
      border-color: var(--primary-color, #007bff);
    }
    
    &:focus + .radio-label {
      box-shadow: 0 0 0 3px rgba(0, 123, 255, 0.1);
    }
  }
}

.radio-label {
  display: inline-block;
  padding: 0.6rem 1.2rem;
  border: 2px solid var(--border-color, #e1e5e9);
  border-radius: 8px;
  font-weight: 500;
  transition: all 0.2s ease;
  background-color: white;
  
  &:hover {
    border-color: var(--primary-hover, #0056b3);
  }
}

.form-actions {
  display: flex;
  gap: 1rem;
  margin-top: 2rem;
}

.btn {
  padding: 0.75rem 1.5rem;
  border: none;
  border-radius: 8px;
  font-size: 1rem;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
  text-decoration: none;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 120px;

  &:disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }

  &--primary {
    background-color: var(--primary-color, #007bff);
    color: white;

    &:hover:not(:disabled) {
      background-color: var(--primary-hover, #0056b3);
      transform: translateY(-1px);
    }
  }

  &--secondary {
    background-color: var(--secondary-color, #6c757d);
    color: white;

    &:hover:not(:disabled) {
      background-color: var(--secondary-hover, #545b62);
      transform: translateY(-1px);
    }
  }
}

.status-message {
  margin-top: 1.5rem;
  padding: 1rem;
  border-radius: 8px;
  font-weight: 500;
  text-align: center;
  
  &.success {
    background-color: var(--success-bg, #d4edda);
    color: var(--success-text, #155724);
    border: 1px solid var(--success-border, #c3e6cb);
  }
  
  &.error {
    background-color: var(--error-bg, #f8d7da);
    color: var(--error-text, #721c24);
    border: 1px solid var(--error-border, #f5c6cb);
  }
}

// Responsive design
@media (max-width: 768px) {
  .form-row {
    flex-direction: column;
  }
  
  .form-col {
    flex: 1 0 100%;
  }
  
  .form-actions {
    flex-direction: column;
  }
  
  .btn {
    width: 100%;
  }
  
  .radio-group {
    flex-direction: column;
  }
}
</style>