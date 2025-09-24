<template>
    <div class="card card--big">
        <div class="card__header">
            <h2 class="card__title">Wybierz Profil</h2>
        </div>
        <div class="card__body">
            <div class="card__content">

                <!-- Loading / Error / Empty -->
                <div v-if="loading" class="loading-text">Ładowanie profilów...</div>
                <div v-else-if="error" class="error-text">{{ error }}</div>
                <div v-else-if="profiles.length === 0" class="empty-text">Brak zapisanych profilów</div>

                <!-- Profile Selector -->
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

                    <!-- Profile Details -->
                    <div class="profile-details" v-if="selectedProfile">
                        <h3>Szczegóły profilu</h3>

                        <!-- WLED -->
                        <div class="detail-section">
                            <h4>WLED:</h4>
                            <p v-if="!wled">Nie ustawiono</p>
                            <template v-else>
                                <p v-if="wled.on === false">Status: Wyłączony</p>
                                <p v-else-if="wled.lor === 0">Status: Ambilight</p>
                                <p v-else-if="wled.preset_name">Preset: {{ wled.preset_name }}</p>
                                <p v-else-if="wled.ps != null">Preset ID: {{ wled.ps }}</p>
                                <p v-else>Status: Włączony</p>
                            </template>
                        </div>

                        <!-- Lights -->
                        <div class="detail-section">
                            <h4>Oświetlenie:</h4>
                            <p v-if="lights">{{ lights.payload.state ? 'Włączone' : 'Wyłączone' }}</p>
                            <p v-else>Nie ustawiono</p>
                        </div>

                        <!-- AC -->
                        <div class="detail-section">
                            <h4>Klimatyzacja:</h4>
                            <p v-if="ac">Temperatura: {{ ac.payload.requestedTemp }}°C</p>
                            <p v-else>Nie ustawiono</p>
                        </div>

                        <!-- Blinds -->
                        <div class="detail-section">
                            <h4>Rolety:</h4>
                            <p v-if="blinds?.state === 'open'">Status: Otwarte</p>
                            <p v-else-if="blinds?.state === 'close'">Status: Zamknięte</p>
                            <p v-else-if="blinds?.automate">
                                Status: Automatyczne (Min: {{ blinds.minLux }}, Max: {{ blinds.maxLux }})
                            </p>
                            <p v-else>Nie ustawiono</p>
                        </div>

                        <!-- Actions -->
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
import { ref, computed, onMounted, watch } from 'vue';
import { useAutomateStore } from '@/stores/automateStore';
import { useLinkStore } from '@/stores/linkStore';
import { useWsStore } from '@/stores/wsStore';

const automateStore = useAutomateStore();
const linkStore = useLinkStore();
const wsStore = useWsStore();

const props = defineProps({
    sharedProfileNames: { type: Set, default: () => new Set() }
});
const emit = defineEmits(['update-profiles']);

const profiles = ref([]);
const loading = ref(true);
const error = ref(null);
const selectedProfileId = ref('');

const selectedProfile = computed(() =>
    profiles.value.find(p => p.profile_id === selectedProfileId.value) || null
);

// --- Computed dla sekcji ---
const wled = computed(() => selectedProfile.value?.profile_json?.WLED);
const lights = computed(() => selectedProfile.value?.profile_json?.lights);
const ac = computed(() => selectedProfile.value?.profile_json?.AC);
const blinds = computed(() => selectedProfile.value?.profile_json?.blinds);

// --- WebSocket helper ---
function sendViaWebSocket(payload) {
    const ws = new WebSocket(wsStore.wsUrl);
    ws.onopen = () => ws.send(JSON.stringify(payload));
    ws.onmessage = ws.onerror = ws.onclose = () => ws.close();
}

// --- Fetch profiles ---
async function fetchProfiles() {
    loading.value = true;
    error.value = null;
    try {
        const response = await fetch(linkStore.getPhpApiUrl('chooseProfile.php'));
        if (!response.ok) throw new Error(`HTTP error! Status: ${response.status}`);
        const data = await response.json();
        if (data.success) {
            profiles.value = data.profiles;
            emit('update-profiles', new Set(data.profiles.map(p => p.profile_name)));
        } else {
            throw new Error(data.error || 'Failed to fetch profiles');
        }
    } catch (err) {
        error.value = `Błąd podczas ładowania profilów: ${err.message}`;
    } finally {
        loading.value = false;
    }
}

// --- Apply profile ---
async function applyProfile() {
    if (!selectedProfile.value) return;
    const profile = selectedProfile.value.profile_json;

    const handlers = {
        WLED: (w) => linkStore.sendWledCommand(w),
        lights: (l) => sendViaWebSocket({ channel: l.channel, lightON: l.payload.state }),
        AC: (a) => sendViaWebSocket(a),
        blinds: (b) => applyBlindsPayload(b)
    };

    for (const [key, fn] of Object.entries(handlers)) {
        if (profile?.[key]) await fn(profile[key]);
    }

    alert('Profil został zastosowany!');
}

// --- Delete profile ---
async function deleteProfile() {
    if (!selectedProfile.value) return;
    if (!confirm(`Czy na pewno chcesz usunąć profil "${selectedProfile.value.profile_name}"?`)) return;
    try {
        const response = await fetch(linkStore.getPhpApiUrl('deleteProfile.php'), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ profile_id: selectedProfile.value.profile_id })
        });
        const data = await response.json();
        if (data.success) {
            profiles.value = profiles.value.filter(p => p.profile_id !== selectedProfile.value.profile_id);
            selectedProfileId.value = '';
            emit('update-profiles', new Set(profiles.value.map(p => p.profile_name)));
            alert('Profil został usunięty!');
        } else {
            throw new Error(data.error || 'Nie udało się usunąć profilu');
        }
    } catch (err) {
        alert(`Błąd podczas usuwania profilu: ${err.message}`);
    }
}

// --- Blinds payload ---
async function applyBlindsPayload(payload) {
    if (typeof payload.state === 'string') {
        automateStore.automate_flag = false;
        await fetch(linkStore.getPhpApiUrl('setBlindsAutomate.php'), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ automate: false })
        });
        await fetch(linkStore.getPhpApiUrl('tuyaBlindsApi.php') + `?action=${payload.state}`);
        return;
    }

    if (typeof payload.maxLux === 'number' && typeof payload.minLux === 'number' && typeof payload.automate === 'boolean') {
        automateStore.automate_flag = payload.automate;
        await fetch(linkStore.getPhpApiUrl('saveBlindsConfig.php'), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ min_lux: payload.minLux, max_lux: payload.maxLux, automate: payload.automate })
        });
    }
}

// --- Lifecycle ---
onMounted(fetchProfiles);

// --- Watch sharedProfileNames ---
watch(() => props.sharedProfileNames, async (newNames) => {
    const currentNames = new Set(profiles.value.map(p => p.profile_name));
    const needsRefresh = [...newNames].some(name => !currentNames.has(name));
    if (needsRefresh) await fetchProfiles();
}, { deep: true });
</script>

<style lang="scss" scoped>
.loading-text,
.error-text,
.empty-text {
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
