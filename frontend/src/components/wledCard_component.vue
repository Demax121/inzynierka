<template>
    <!-- Card wrapper uses global .card layout styles -->
    <div class="card">
        <!-- Header: static title for WLED management -->
        <div class="card__header">
            <h2 class="card__title">WLED Control</h2>
        </div>
        <div class="card__body">
            <div class="card__content">
                <!-- Primary action buttons
                     WLED Mode: enables WLED with a specific preset (lor:2 likely local override / realtime off, ps:1 preset id)
                     Ambilight Mode: sets lor:0 (live override / screen capture style mode) keeping LEDs on
                     Turn off LED: powers device off (on:false)
                     All buttons disable while loading (preset fetch or command in-flight) -->
                <div class="button-group">
                    <button class="btn" @click="sendCommand({
                        on: true,
                        lor: 2,
                        ps: 1
                    })" 
                    :disabled="loading">
                        {{ 'WLED Mode' }}
                    </button>

                    <button class="btn" @click="sendCommand({ on: true, lor: 0 })" :disabled="loading">
                        {{ 'Ambilight Mode' }}
                    </button>

                    <button class="btn" @click="sendCommand({ on: false })" :disabled="loading">
                        {{ 'Turn off LED' }}
                    </button>
                </div>
                
                <!-- Presets section: populated asynchronously from WLED JSON API via linkStore -->
                <div class="presets-container">
                    <!-- Inline loading state shown until first preset payload arrives -->
                    <div v-if="loading && !presetsLoaded" class="loading-indicator">
                        Loading presets...
                    </div>
                    
                    <!-- Dropdown & apply button appear once presets array is non-empty -->
                    <div v-if="presets.length > 0" class="preset-select-container">
                        <select v-model="selectedPreset" class="preset-select" :disabled="loading">
                            <option disabled value="">Select preset</option>
                            <!-- Regular presets from WLED (id + display name) -->
                            <option v-for="preset in presets" :key="preset.id" :value="preset.id">
                                {{ preset.name }}
                            </option>
                        </select>
                        
                        <!-- Apply uses applySelectedPreset which branches on special sentinel values (off / ambilight) -->
                        <button class="btn" @click="applySelectedPreset" :disabled="!selectedPreset || loading">
                            Apply
                        </button>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
// WLED control card
// Responsibilities:
// 1. Fetch preset list from WLED device through linkStore
// 2. Provide quick mode buttons (standard WLED preset, Ambilight / realtime, power off)
// 3. Allow selecting and applying any preset from dropdown
// 4. Route all outbound commands via linkStore abstraction (centralizes fetch base URL & error handling)
//
// Key WLED JSON API fields used here:
// - on: boolean (power state)
// - ps: number (preset slot to activate)
// - lor: live override / realtime control flag (0 = enable realtime like Ambilight; 2 = normal preset mode)
//
// Special sentinel values (handled in applySelectedPreset):
// - 'off' => power off
// - 'ambilight' => turn on with lor:0
// - any other => treat as numeric preset id (ps)
import { ref, computed, onMounted } from 'vue';
import { useLinkStore } from '@/stores/linkStore';

// Store providing WLED endpoint utilities & cached state
const linkStore = useLinkStore();

// Currently selected preset (string because <select> binds values as strings)
const selectedPreset = ref("");

// Reactive computed wrappers for store state (simplifies template & avoids deep store references)
const loading = computed(() => linkStore.wledPresetsLoading);      // true while initial fetch or command pending
const presetsLoaded = computed(() => linkStore.wledPresetsLoaded); // becomes true after successful fetch
const presets = computed(() => linkStore.wledPresets);             // array of { id, name }

// On mount: trigger initial preset fetch
onMounted(() => {
    linkStore.fetchWledPresets();
});

// Thin command forwarder to keep template concise
function sendCommand(payload) {
    linkStore.sendWledCommand(payload);
}

// Apply whichever preset / special action the user selected in dropdown
function applySelectedPreset() {
    if (!selectedPreset.value) return; // Guard: nothing chosen

    console.log(`Applying selection: ${selectedPreset.value}`);

    // Branch on sentinel keywords first
    if (selectedPreset.value === 'off') {
        sendCommand({ on: false });
        return;
    }
    if (selectedPreset.value === 'ambilight') {
        sendCommand({ on: true, lor: 0 }); // Realtime / Ambilight mode
        return;
    }

    // Fallback: treat as a numeric preset id for ps
    sendCommand({ on: true, ps: selectedPreset.value });
}
</script>

<style lang="scss" scoped>
// Layout wrappers
.presets-container { // vertical stack for loading + select row
    margin-top: 1rem;
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
}

.preset-select-container { // row containing <select> + Apply button
    display: flex;
    gap: 0.5rem;
    margin-top: 0.5rem;
    align-items: center;
}

// Native select styling aligned with app theme tokens (CSS vars)
.preset-select {
    flex: 1;
    padding: 0.5rem;
    border-radius: 0.25rem;
    border: 1px solid #ccc; // consider extracting to a token if reused
    background-color: var(--color-background);
    color: var(--color-text);
    min-width: 150px;
}

// Inline loading message (kept lightweight; can be replaced with shared <LoadingCard /> if desired)
.loading-indicator {
    font-style: italic;
    color: var(--color-text-secondary, #777);
    margin-bottom: 0.5rem;
    text-align: center;
}
</style>