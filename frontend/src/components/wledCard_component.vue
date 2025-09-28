<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">WLED Control</h2>
        </div>
        <div class="card__body">
            <div class="card__content">
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
                
                <div class="presets-container">
                    <div v-if="loading && !presetsLoaded" class="loading-indicator">
                        Loading presets...
                    </div>
                    
                    <div v-if="presets.length > 0" class="preset-select-container">
                        <select v-model="selectedPreset" class="preset-select" :disabled="loading">
                            <option disabled value="">Select preset</option>
                            <!-- Regular presets from WLED -->
                            <option v-for="preset in presets" :key="preset.id" :value="preset.id">
                                {{ preset.name }}
                            </option>
                        </select>
                        
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
import { ref, computed, onMounted } from 'vue';
import { useLinkStore } from '@/stores/linkStore';

// Use the linkStore for WLED functionality
const linkStore = useLinkStore();
const selectedPreset = ref("");

// Get loading and presetsLoaded state for template usage
const loading = computed(() => linkStore.wledPresetsLoading);
const presetsLoaded = computed(() => linkStore.wledPresetsLoaded);
const presets = computed(() => linkStore.wledPresets);

onMounted(() => {
    linkStore.fetchWledPresets();
});

function sendCommand(payload) {
    linkStore.sendWledCommand(payload);
}

function applySelectedPreset() {
    if (selectedPreset.value) {
        console.log(`Applying selection: ${selectedPreset.value}`);
        
        // Handle special options
        if (selectedPreset.value === 'off') {
            // Turn off the LEDs
            sendCommand({
                on: false
            });
        } else if (selectedPreset.value === 'ambilight') {
            // Enable Ambilight mode
            sendCommand({
                on: true,
                lor: 0 // Ambilight mode
            });
        } else {
            // Apply regular preset
            sendCommand({
                on: true,   // Turn on the LEDs
                ps: selectedPreset.value  // Apply the selected preset
            });
        }
    }
}
</script>

<style lang="scss" scoped>
.presets-container {
    margin-top: 1rem;
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
}

.preset-select-container {
    display: flex;
    gap: 0.5rem;
    margin-top: 0.5rem;
    align-items: center;
}

.preset-select {
    flex: 1;
    padding: 0.5rem;
    border-radius: 0.25rem;
    border: 1px solid #ccc;
    background-color: var(--color-background);
    color: var(--color-text);
    min-width: 150px;
}

.loading-indicator {
    font-style: italic;
    color: var(--color-text-secondary, #777);
    margin-bottom: 0.5rem;
    text-align: center;
}
</style>