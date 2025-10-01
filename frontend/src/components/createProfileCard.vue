<template>
  <div class="card card--big">
    <div class="card__header">
      <h2 class="card__title">Konfigurator Profili</h2>
    </div>
    <div class="card__body">
      <form class="profile-form" @submit.prevent="saveProfile">
        
        <!-- Nazwa profilu -->
        <div class="form-group">
          <label class="form-label">Nazwa profilu:</label>
          <input v-model="profile.name" class="form-input" placeholder="Wprowadź nazwę profilu" required />
        </div>

        <!-- Klimatyzacja -->
        <div class="form-section">
          <h3 class="form-section__title">Klimatyzacja</h3>
          <label class="form-switch">
            <input type="checkbox" v-model="profile.AC.status">
            <span class="form-switch__slider"></span>
            <span class="form-switch__label">{{ profile.AC.status ? 'Włączona' : 'Wyłączona' }}</span>
          </label>

          <div v-if="profile.AC.status" class="form-group">
            <label class="form-label">Temperatura docelowa (°C):</label>
            <input type="range" v-model.number="profile.AC.targetTemp" min="16" max="30" step="0.5" class="form-slider" />
            <span>{{ profile.AC.targetTemp }}°C</span>
          </div>
        </div>

        <!-- Główne oświetlenie -->
        <div class="form-section">
          <h3 class="form-section__title">Główne oświetlenie</h3>
          <label class="form-switch">
            <input type="checkbox" v-model="profile.lights.state">
            <span class="form-switch__slider"></span>
            <span class="form-switch__label">{{ profile.lights.state ? 'Włączone' : 'Wyłączone' }}</span>
          </label>
        </div>

        <!-- Rolety -->
        <div class="form-section">
          <h3 class="form-section__title">Rolety</h3>
          <div class="radio-group">
            <label class="radio-item">
              <input type="radio" value="open" v-model="blindsMode" />
              <span class="radio-label">Otwarte</span>
            </label>
            <label class="radio-item">
              <input type="radio" value="close" v-model="blindsMode" />
              <span class="radio-label">Zamknięte</span>
            </label>
            <label class="radio-item">
              <input type="radio" value="auto" v-model="blindsMode" />
              <span class="radio-label">Automatycznie</span>
            </label>
          </div>

          <div v-if="blindsMode === 'auto'" class="form-row">
            <div class="form-col">
              <label class="form-label">Min jasność (lux):</label>
              <input type="number" v-model.number="profile.Blinds.minLux" min="0" step="10" class="form-input" required />
            </div>
            <div class="form-col">
              <label class="form-label">Max jasność (lux):</label>
              <input type="number" v-model.number="profile.Blinds.maxLux" min="0" step="10" class="form-input" required />
            </div>
          </div>
        </div>

        <!-- WLED -->
        <div class="form-section">
          <h3 class="form-section__title">Oświetlenie LED</h3>
          <select v-model="wledMode" class="form-select">
            <option value="off">Wyłączony</option>
            <option value="wled">WLED</option>
            <option value="ambilight">Ambilight</option>
          </select>

          <div v-if="wledMode === 'wled'" class="form-group">
            <label class="form-label">Preset efektów:</label>
            <select v-model="selectedPreset" class="form-select">
              <option v-for="preset in wledPresets" :key="preset.id" :value="preset.id">{{ preset.name }}</option>
            </select>
          </div>

          <div v-if="wledMode !== 'off'" class="form-group">
            <label class="form-label">Jasność:</label>
            <input type="range" v-model.number="profile.WLED.bri" min="0" max="255" class="form-slider" />
            <span>{{ Math.round((profile.WLED.bri / 255) * 100) }}%</span>
          </div>
        </div>

        <!-- Akcje -->
        <div class="form-actions">
          <button type="submit" class="btn btn--primary" :disabled="loading">{{ loading ? 'Zapisywanie...' : 'Zapisz profil' }}</button>
          <button type="button" class="btn btn--secondary" @click="resetForm">Wyczyść formularz</button>
        </div>

        <!-- Status -->
        <div v-if="statusMessage" :class="['status-message', statusMessage.type]">{{ statusMessage.text }}</div>
      </form>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, watch, onMounted } from 'vue';
import { useLinkStore } from '@/stores/linkStore';

const linkStore = useLinkStore();
const loading = ref(false);
const statusMessage = ref(null);

const profile = reactive({
  name: '',
  AC: { status: false, targetTemp: 22 },
  lights: { state: false },
  Blinds: { status: 'open', minLux: 100, maxLux: 1000, automate: false },
  WLED: { on: false, bri: 128, fx: 1 }
});

const blindsMode = ref('open');
const wledMode = ref('off');
const selectedPreset = ref(1);
const wledPresets = ref([]);

watch(blindsMode, mode => {
  if (mode === 'open' || mode === 'close') {
    profile.Blinds = { status: mode };
  } else if (mode === 'auto') {
    profile.Blinds = { automate: true, minLux: 100, maxLux: 1000 };
  }
});

watch(wledMode, mode => {
  profile.WLED.on = mode !== 'off';
  profile.WLED.fx = mode === 'ambilight' ? 0 : selectedPreset.value;
});

watch(selectedPreset, preset => {
  if (wledMode.value === 'wled') profile.WLED.fx = preset;
});

// Fetch WLED presets from API
const fetchWledPresets = async () => {
  try {
    const res = await fetch('http://192.168.1.25/json/presets');
    const data = await res.json();
    wledPresets.value = Object.entries(data)
      .filter(([key]) => !isNaN(Number(key)))
      .map(([id, val]) => ({ id: Number(id), name: val.n || `Preset ${id}` }));
    if (wledPresets.value.length) selectedPreset.value = wledPresets.value[0].id;
  } catch {}
};

const saveProfile = async () => {
  loading.value = true;
  statusMessage.value = null;
  try {
    const payload = {
      name: profile.name,
      AC: profile.AC,
      lights: profile.lights,
      Blinds: profile.Blinds,
      WLED: profile.WLED
    };
    const res = await fetch(linkStore.getPhpApiUrl('saveProfile.php'), {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    const data = await res.json();
    statusMessage.value = data.success
      ? { type: 'success', text: 'Profil został zapisany pomyślnie!' }
      : { type: 'error', text: data.message || 'Błąd zapisu profilu' };
  } catch (err) {
    statusMessage.value = { type: 'error', text: err.message };
  } finally {
    loading.value = false;
  }
};

const resetForm = () => {
  profile.name = '';
  profile.AC = { status: false, targetTemp: 22 };
  profile.lights.state = false;
  blindsMode.value = 'open';
  profile.Blinds = { status: 'open', minLux: 100, maxLux: 1000, automate: false };
  wledMode.value = 'off';
  profile.WLED = { on: false, bri: 128, fx: 1 };
  selectedPreset.value = 1;
  statusMessage.value = null;
};

onMounted(fetchWledPresets);
</script>
