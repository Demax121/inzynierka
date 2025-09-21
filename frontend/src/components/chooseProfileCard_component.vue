<template>
    <div class="card card--big">
        <div class="card__header">
            <h2 class="card__title">Wybierz Profil</h2>
        </div>
        <div class="card__body">
            <div class="card__content">
                <div v-if="loading" class="loading-text">
                    Ładowanie profilów...
                </div>
                <div v-else-if="error" class="error-text">
                    {{ error }}
                </div>
                <div v-else-if="profiles.length === 0" class="empty-text">
                    Brak zapisanych profilów
                </div>
                <div v-else class="profile-selector">
                    <div class="profile-dropdown">
                        <label for="profile-select" class="profile-label">Wybierz profil:</label>
                        <select id="profile-select" v-model="selectedProfileId" class="profile-select">
                            <option disabled value="">-- Wybierz profil --</option>
                            <option v-for="profile in profiles" :key="profile.profile_id" :value="profile.profile_id">
                                {{ profile.profile_name }}
                            </option>
                        </select>
                    </div>
                    
                    <div class="profile-details" v-if="selectedProfile">
                        <h3>Szczegóły profilu</h3>
                        <div class="detail-section">
                            <h4>WLED:</h4>
                            <div v-if="selectedProfile.profile_json && selectedProfile.profile_json.WLED" class="wled-info">
                                <p v-if="selectedProfile.profile_json.WLED.on === false">Status: Wyłączony</p>
                                <p v-else-if="selectedProfile.profile_json.WLED.lor === 0">Status: Ambilight</p>
                                <p v-else-if="selectedProfile.profile_json.WLED.preset_name">
                                    Preset: {{ selectedProfile.profile_json.WLED.preset_name }}
                                </p>
                                <p v-else-if="selectedProfile.profile_json.WLED.ps !== null && selectedProfile.profile_json.WLED.ps !== undefined">
                                    Preset ID: {{ selectedProfile.profile_json.WLED.ps }}
                                </p>
                                <p v-else>Status: Włączony</p>
                            </div>
                            <p v-else>Nie ustawiono</p>
                        </div>
                        
                        <div class="detail-section">
                            <h4>Oświetlenie:</h4>
                            <p v-if="selectedProfile.profile_json && selectedProfile.profile_json.lights">
                                {{ selectedProfile.profile_json.lights.payload.state ? 'Włączone' : 'Wyłączone' }}
                            </p>
                            <p v-else>Nie ustawiono</p>
                        </div>
                        
                        <div class="detail-section">
                            <h4>Klimatyzacja:</h4>
                            <p v-if="selectedProfile.profile_json && selectedProfile.profile_json.AC">
                                Temperatura: {{ selectedProfile.profile_json.AC.payload.requestedTemp }}°C
                            </p>
                            <p v-else>Nie ustawiono</p>
                        </div>
                        
                        <div class="detail-section">
                            <h4>Rolety:</h4>
                            <p v-if="selectedProfile.profile_json && selectedProfile.profile_json.blinds && selectedProfile.profile_json.blinds.state === 'open'">
                                Status: Otwarte
                            </p>
                            <p v-else-if="selectedProfile.profile_json && selectedProfile.profile_json.blinds && selectedProfile.profile_json.blinds.state === 'close'">
                                Status: Zamknięte
                            </p>
                            <p v-else-if="selectedProfile.profile_json && selectedProfile.profile_json.blinds && selectedProfile.profile_json.blinds.automate">
                                Status: Automatyczne (Min: {{ selectedProfile.profile_json.blinds.minLux }}, Max: {{ selectedProfile.profile_json.blinds.maxLux }})
                            </p>
                            <p v-else>Nie ustawiono</p>
                        </div>
                        
                        <div class="profile-actions">
                            <button class="btn btn-apply" @click="applyProfile">Zastosuj profil</button>
                            <button class="btn btn-delete" @click="deleteProfile">Usuń</button>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, watch } from 'vue';
import { useLinkStore } from '@/stores/linkStore';

// Get linkStore for API access
const linkStore = useLinkStore();

// Define props to receive shared profile names from parent
const props = defineProps({
    sharedProfileNames: {
        type: Set,
        default: () => new Set()
    }
});

// Define emits to send updates to parent
const emit = defineEmits(['update-profiles']);

// Component state
const profiles = ref([]);
const loading = ref(true);
const error = ref(null);
const selectedProfileId = ref('');

// Computed property to get selected profile details
const selectedProfile = computed(() => {
    if (!selectedProfileId.value) return null;
    const profile = profiles.value.find(profile => profile.profile_id === selectedProfileId.value);
    
    if (!profile) return null;
    
    // No need to parse JSON as the backend now returns it already parsed
    return profile;
});

// Fetch profiles from the database
async function fetchProfiles() {
    loading.value = true;
    error.value = null;
    
    try {
        const response = await fetch(linkStore.getPhpApiUrl('chooseProfile.php'));
        
        if (!response.ok) {
            throw new Error(`HTTP error! Status: ${response.status}`);
        }
        
        const data = await response.json();
        
        if (data.success) {
            profiles.value = data.profiles;
            
            // Update shared profile names
            const profileNameSet = new Set();
            profiles.value.forEach(profile => {
                profileNameSet.add(profile.profile_name);
            });
            emit('update-profiles', profileNameSet);
            
        } else {
            throw new Error(data.error || 'Failed to fetch profiles');
        }
    } catch (err) {
        console.error('Error fetching profiles:', err);
        error.value = `Błąd podczas ładowania profilów: ${err.message}`;
    } finally {
        loading.value = false;
    }
}

// Apply the selected profile
async function applyProfile() {
    if (!selectedProfile.value) return;
    
    const profile = selectedProfile.value.profile_json;
    
    // Apply each component of the profile
    
    // 1. Apply WLED settings if present
    if (profile && profile.WLED) {
        try {
            await linkStore.sendWledCommand(profile.WLED);
            console.log('Applied WLED settings');
        } catch (err) {
            console.error('Failed to apply WLED settings:', err);
        }
    }
    
    // 2. Apply lights settings if present (implement this functionality)
    if (profile && profile.lights) {
        console.log('Would apply lights settings:', profile.lights);
        // TODO: Implement lights control via WebSocket
    }
    
    // 3. Apply AC settings if present (implement this functionality)
    if (profile && profile.AC) {
        console.log('Would apply AC settings:', profile.AC);
        // TODO: Implement AC control via WebSocket
    }
    
    // 4. Apply blinds settings if present (implement this functionality)
    if (profile && profile.blinds) {
        console.log('Would apply blinds settings:', profile.blinds);
        // TODO: Implement blinds control via WebSocket or API
    }
    
    alert('Profil został zastosowany!');
}

// Delete the selected profile
async function deleteProfile() {
    if (!selectedProfile.value) return;
    if (!confirm(`Czy na pewno chcesz usunąć profil "${selectedProfile.value.profile_name}"?`)) return;
    
    try {
        // Call the API to delete the profile
        const response = await fetch(linkStore.getPhpApiUrl('deleteProfile.php'), {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                profile_id: selectedProfile.value.profile_id
            })
        });
        
        const data = await response.json();
        
        if (data.success) {
            // Remove from local list
            const deletedProfileName = selectedProfile.value.profile_name;
            profiles.value = profiles.value.filter(p => p.profile_id !== selectedProfile.value.profile_id);
            selectedProfileId.value = '';
            
            // Update shared profile names
            const updatedNames = new Set();
            profiles.value.forEach(profile => {
                updatedNames.add(profile.profile_name);
            });
            emit('update-profiles', updatedNames);
            
            alert('Profil został usunięty!');
        } else {
            throw new Error(data.error || 'Nie udało się usunąć profilu');
        }
    } catch (err) {
        console.error('Error deleting profile:', err);
        alert(`Błąd podczas usuwania profilu: ${err.message}`);
    }
}

// Fetch profiles on component mount
onMounted(() => {
    fetchProfiles();
});

// Watch for changes in props.sharedProfileNames
watch(() => props.sharedProfileNames, async (newNames) => {
    // If a new profile was added externally, refresh profiles list
    const currentProfileNames = new Set(profiles.value.map(p => p.profile_name));
    let needsRefresh = false;
    
    // Check if any new profile names are in the shared list
    for (const name of newNames) {
        if (!currentProfileNames.has(name)) {
            needsRefresh = true;
            break;
        }
    }
    
    // If we found a new profile name, refresh the list
    if (needsRefresh) {
        await fetchProfiles();
    }
}, { deep: true });
</script>

<style lang="scss" scoped>
.loading-text, .error-text, .empty-text {
    padding: 1.5rem;
    text-align: center;
}

.error-text {
    color: #d9534f;
}

.profile-selector {
    display: flex;
    flex-direction: column;
    gap: 1.5rem;
}

.profile-dropdown {
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
}

.profile-label {
    font-weight: 600;
    font-size: 1.1rem;
}

.profile-select {
    padding: 0.75rem;
    font-size: 1rem;
    border-radius: 4px;
    border: 1px solid #ccc;
    background-color: var(--color-background);
    color: var(--color-text);
}

.profile-details {
    margin-top: 1rem;
    padding: 1rem;
    border: 1px solid #ddd;
    border-radius: 8px;
    background-color: rgba(255, 255, 255, 0.05);
    
    h3 {
        margin-top: 0;
        margin-bottom: 1rem;
        font-size: 1.2rem;
        color: var(--color-heading);
    }
}

.detail-section {
    margin-bottom: 1rem;
    padding-bottom: 0.75rem;
    border-bottom: 1px solid rgba(255, 255, 255, 0.1);
    
    &:last-child {
        border-bottom: none;
    }
    
    h4 {
        margin: 0.5rem 0;
        font-size: 1rem;
        color: var(--color-heading);
    }
    
    p {
        margin: 0.25rem 0;
        font-size: 0.9rem;
    }
}

.profile-actions {
    display: flex;
    justify-content: space-between;
    margin-top: 1.5rem;
    gap: 1rem;
    
    button {
        flex: 1;
    }
}

.btn {
    padding: 0.6rem 1.2rem;
    font-size: 0.9rem;
    border: none;
    border-radius: 4px;
    cursor: pointer;
    transition: background-color 0.2s, transform 0.1s;
    
    &:hover {
        filter: brightness(1.1);
    }
    
    &:active {
        transform: translateY(1px);
    }
}

.btn-apply {
    background-color: #4CAF50;
    color: white;
}

.btn-delete {
    background-color: #f44336;
    color: white;
}
</style>