<template>
  <!-- Preserve global card; introduce local BEM block 'create-profile' -->
  <div class="card card--big create-profile">
    <div class="card__header">
      <h2 class="card__title">Profile generator</h2>
    </div>
    <div class="card__body">
      <div class="card__content create-profile__content">
        <!-- Main form -->
        <form @submit.prevent="saveProfile" class="create-profile__form" ref="formRef">

          <!-- Profile name -->
          <div class="create-profile__group create-profile__group--name">
            <label for="profileName" class="create-profile__label">Profile Name:</label>
            <input
              type="text"
              id="profileName"
              v-model="profileName"
              class="create-profile__control"
              placeholder="Wpisz nazwę profilu"
              required
            >
            <p v-if="profileNameError" class="create-profile__error">{{ profileNameError }}</p>
          </div>

          <!-- WLED preset selection -->
            <div class="create-profile__group create-profile__group--wled">
            <label for="wledPreset" class="create-profile__label">WLED Preset:</label>
            <div class="create-profile__preset-wrapper">
              <div v-if="loading && !presetsLoaded" class="create-profile__loading">Loading presets...</div>
              <select
                v-if="presets.length > 0 || presetsLoaded"
                v-model="profileSettings.wledPreset"
                id="wledPreset"
                class="create-profile__control create-profile__control--select"
                required
              >
                <option disabled value="">Choose WLED Preset</option>
                <option value="off">OFF</option>
                <option value="ambilight">Ambilight</option>
                <option v-for="preset in presets" :key="preset.id" :value="preset.id">
                  {{ preset.name }}
                </option>
              </select>
            </div>
          </div>

          <!-- Main lights toggle -->
          <div class="create-profile__group create-profile__group--lights">
            <label for="mainLights" class="create-profile__label">Main Lights:</label>
            <div class="create-profile__lights">
              <!-- Use global switch component (small variant) from buttons.scss -->
              <label class="switch switch--small" :aria-label="profileSettings.lightsOn ? 'Disable main lights' : 'Enable main lights'">
                <input type="checkbox" v-model="profileSettings.lightsOn" id="mainLights">
                <span class="slider"></span>
              </label>
            </div>
          </div>

          <!-- AC control -->
          <div class="create-profile__group create-profile__group--ac">
            <label for="acTemperature" class="create-profile__label">AC Control:</label>
            <div class="create-profile__ac">
              <input
                type="number"
                id="acTemperature"
                v-model.number="profileSettings.acTemperature"
                min="16"
                max="30"
                step="1"
                placeholder="Set temperature (16-30°C)"
                class="create-profile__control create-profile__control--temperature"
                required
              >
            </div>
          </div>

          <!-- Blinds configuration -->
          <div class="create-profile__group create-profile__group--blinds">
            <label class="create-profile__label">Window Blinds:</label>
            <div class="create-profile__blinds">
              <div class="create-profile__radio-group">
                <label class="create-profile__radio">
                  <input type="radio" v-model="profileSettings.blindsMode" value="open" name="blinds-mode">
                  <span>Open</span>
                </label>
                <label class="create-profile__radio">
                  <input type="radio" v-model="profileSettings.blindsMode" value="close" name="blinds-mode">
                  <span>Closed</span>
                </label>
                <label class="create-profile__radio">
                  <input type="radio" v-model="profileSettings.blindsMode" value="auto" name="blinds-mode">
                  <span>Automate</span>
                </label>
              </div>
              <!-- Auto mode lux thresholds -->
              <div v-if="profileSettings.blindsMode === 'auto'" class="create-profile__lux">
                <div class="create-profile__help">Set treshold limits for this profile below:</div>
                <div class="create-profile__lux-config">
                  <div class="create-profile__input-row">
                    <label for="minLux">Min lux:</label>
                    <input id="minLux" v-model.number="profileSettings.minLux" type="number" class="create-profile__lux-input" required>
                    <span class="create-profile__lux-note">Above this treshold blinds will open</span>
                  </div>
                  <div class="create-profile__input-row">
                    <label for="maxLux">Max lux:</label>
                    <input id="maxLux" v-model.number="profileSettings.maxLux" type="number" class="create-profile__lux-input" required>
                    <span class="create-profile__lux-note">Above this treshold blinds will be closed</span>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <!-- Hidden JSON field (not strictly needed for XHR based submit, kept for compatibility) -->
          <input type="hidden" name="profileJSON" :value="JSON.stringify(buildProfileJSON())" />

          <!-- Submit button -->
          <div class="create-profile__actions">
            <button type="submit" class="create-profile__btn" ref="saveButtonRef">Save profile</button>
          </div>
        </form>
        <dialog-box ref="dialogRef" />
      </div>
    </div>
  </div>
</template>

<script setup>
// Component: Create Profile
// Provides a form to build a profile JSON compatible with backend schema.
// Features:
// - Validates profile name (client mirror of backend regex)
// - Dynamically builds JSON according to WLED / lights / AC / blinds selections
// - Sends POST to createProfile.php and informs parent via emit
// - Uses refs instead of querySelector for button state management

import { ref, reactive, computed, onMounted } from 'vue';
import { useLinkStore } from '@/stores/linkStore';
import dialogBox from '@/components/dialogBox_component.vue'

const linkStore = useLinkStore();

// --- Presets reactive proxies ---
const loading = computed(() => linkStore.wledPresetsLoading);
const presetsLoaded = computed(() => linkStore.wledPresetsLoaded);
const presets = computed(() => linkStore.wledPresets);

// --- Form state ---
const profileName = ref('');
const profileSettings = reactive({
  wledPreset: '',
  lightsOn: false,
  acTemperature: null,
  blindsMode: 'open',
  minLux: 5000,
  maxLux: 20000,
});

// --- Validation regex (keep in sync with backend) ---
const NAME_REGEX = /^[A-Za-z0-9 _-]{1,64}$/;
const profileNameError = ref('');

// --- Refs to DOM elements (avoid querySelector) ---
const saveButtonRef = ref(null);
const formRef = ref(null);
const dialogRef = ref(null);

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

// Build the JSON structure for submission; matches docs/json-payloads spec.
function buildProfileJSON() {
  const profileJSON = {
    WLED: { on: true, lor: 2, ps: null },
    lights: { channel: 'main_lights', payload: { state: profileSettings.lightsOn } }
  };

  if (profileSettings.acTemperature !== null && profileSettings.acTemperature !== '') {
    profileJSON.AC = { channel: 'air_conditioning', payload: { requestedTemp: profileSettings.acTemperature } };
  }

  if (profileSettings.blindsMode === 'open' || profileSettings.blindsMode === 'close') {
    profileJSON.blinds = { state: profileSettings.blindsMode };
  } else if (profileSettings.blindsMode === 'auto') {
    profileJSON.blinds = { minLux: profileSettings.minLux, maxLux: profileSettings.maxLux, automate: true };
  }

  // WLED variants
  if (profileSettings.wledPreset === 'off') {
    profileJSON.WLED.on = false;
    profileJSON.WLED.preset_name = 'OFF';
    delete profileJSON.WLED.lor; delete profileJSON.WLED.ps;
  } else if (profileSettings.wledPreset === 'ambilight') {
    profileJSON.WLED.on = true; profileJSON.WLED.lor = 0; profileJSON.WLED.preset_name = 'Ambilight'; delete profileJSON.WLED.ps;
  } else if (profileSettings.wledPreset) {
    profileJSON.WLED.on = true; profileJSON.WLED.lor = 2; profileJSON.WLED.ps = parseInt(profileSettings.wledPreset);
    const presetObj = presets.value.find(p => p.id.toString() === profileSettings.wledPreset.toString());
    profileJSON.WLED.preset_name = presetObj ? presetObj.name : `Preset ${profileSettings.wledPreset}`;
  }
  return profileJSON;
}

const props = defineProps({
  sharedProfileNames: { type: Object, default: () => new Set() }
});
const emit = defineEmits(['profile-created']);

function resetFormState() {
  profileName.value = '';
  profileSettings.wledPreset = '';
  profileSettings.lightsOn = false;
  profileSettings.acTemperature = null;
  profileSettings.blindsMode = 'open';
  profileSettings.minLux = 5000;
  profileSettings.maxLux = 20000;
}

function saveProfile() {
  if (!validateProfileName()) { dialogRef.value?.openMessage(profileNameError.value); return; }
  if (props.sharedProfileNames.has(profileName.value.trim())) { dialogRef.value?.openMessage('Profile with this name already exists.'); return; }

  const profileData = buildProfileJSON();
  const saveData = { profile_name: profileName.value.trim(), profile_json: profileData };

  const btn = saveButtonRef.value;
  const originalText = btn.innerText;
  btn.innerText = 'Zapisywanie...'; btn.disabled = true;

  fetch(linkStore.getPhpApiUrl('createProfile.php'), {
    method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(saveData)
  })
    .then(r => r.json())
    .then(d => {
      if (d.success) {
        dialogRef.value?.openMessage('Profile has been created succesfuly');
        const updated = new Set(props.sharedProfileNames); updated.add(profileName.value.trim());
        emit('profile-created', updated); resetFormState();
      } else {
        dialogRef.value?.openMessage('Error: ' + (d.error || 'Profile could not be saved'));
      }
    })
    .catch(e => dialogRef.value?.openMessage('Connection error: ' + e.message))
    .finally(() => { btn.innerText = originalText; btn.disabled = false; });
}

onMounted(() => { linkStore.fetchWledPresets(); });
</script>

<style lang="scss" scoped>
// Block: create-profile
// Purpose: encapsulate profile creation form styling without touching global card styles

$cp-spacing: 1rem;
$cp-radius: 0.25rem;
$cp-border: #ccc;
$cp-muted: var(--color-text-secondary, #777);
$cp-accent: var(--color-accent, #4CAF50);
$cp-error: #ff4d4f;
$cp-transition: 0.25s ease;

.create-profile {
  &__content { display:flex; flex-direction:column; }
  &__form { display:flex; flex-direction:column; gap:1.25rem; }
  &__group { display:flex; flex-direction:column; gap:0.4rem; }
  &__label { font-weight:600; font-size:0.95rem; }
  &__error { color:$cp-error; font-size:0.75rem; margin:0; }
  &__control {
    width:100%; padding:0.55rem 0.65rem; border:1px solid $cp-border; border-radius:$cp-radius;
    background:var(--color-background); color:var(--color-text); font-size:0.9rem;
    transition:border-color $cp-transition, box-shadow $cp-transition;
    &:focus { outline:none; border-color:$cp-accent; box-shadow:0 0 0 2px rgba(76,175,80,0.25); }
    &--select { cursor:pointer; }
    &--temperature { max-width:12rem; }
  }

  /* Lights switch container (global .switch styles supply the visual switch) */
  &__lights { display:flex; align-items:center; }

  /* Blinds area */
  &__blinds { display:flex; flex-direction:column; gap:0.75rem; }
  &__radio-group { display:flex; gap:1.25rem; flex-wrap:wrap; }
  &__radio { display:flex; align-items:center; gap:0.4rem; cursor:pointer; user-select:none; font-size:0.85rem; }
  &__lux { margin-top:0.25rem; padding-top:0.5rem; border-top:1px dashed $cp-border; }
  &__help { font-size:0.8rem; color:$cp-muted; font-weight:600; text-decoration:underline; margin:0 0 0.5rem; }
  &__lux-config { display:flex; flex-direction:column; gap:0.75rem; }
  &__input-row { display:flex; align-items:center; gap:0.75rem; flex-wrap:wrap; }
  &__lux-input { padding:6px 10px; font-size:0.8rem; width:120px; border:2px solid $cp-border; border-radius:4px; outline:none; transition:border-color $cp-transition; }
  &__lux-input:focus { border-color:$cp-accent; }
  &__lux-note { font-size:0.7rem; color:$cp-muted; flex:1; }

  /* Actions */
  &__actions { display:flex; justify-content:flex-end; }
  &__btn { background:$cp-accent; color:#fff; font-weight:600; border:none; padding:0.7rem 1.25rem; border-radius:$cp-radius; cursor:pointer; font-size:0.85rem; transition:filter $cp-transition, transform $cp-transition; }
  &__btn:hover { filter:brightness(1.1); }
  &__btn:active { transform:translateY(1px); }
}
</style>