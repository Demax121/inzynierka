<template>
    <!-- Maintain global card structure; introduce local BEM block 'choose-profile' for internal elements -->
    <div class="card card--big choose-profile">
        <div class="card__header">
            <h2 class="card__title">Choose Profile</h2>
        </div>
        <div class="card__body">
            <div class="card__content choose-profile__content">

                <!-- Loading / Error / Empty states (mutually exclusive) -->
                <div v-if="loading" class="choose-profile__state choose-profile__state--loading">Loading Profiles...</div>
                <div v-else-if="error" class="choose-profile__state choose-profile__state--error">{{ error }}</div>
                <div v-else-if="profiles.length === 0" class="choose-profile__state choose-profile__state--empty">No profiles to show</div>

                <!-- Selector + details -->
                <div v-else class="choose-profile__selector">
                    <!-- Dropdown wrapper -->
                    <div class="choose-profile__dropdown">
                        <label for="profile-select" class="choose-profile__label">Choose profile:</label>
                        <select id="profile-select" v-model="selectedProfileId" class="choose-profile__select">
                            <option disabled value="">-- Choose Profile --</option>
                            <option v-for="profile in profiles" :key="profile.profile_id" :value="profile.profile_id">
                                {{ profile.profile_name }}
                            </option>
                        </select>
                    </div>

                    <!-- Details Panel -->
                    <div class="choose-profile__details" v-if="selectedProfile">
                        <h3 class="choose-profile__details-title">Profile configuration settings</h3>

                        <!-- WLED Section -->
                        <section class="choose-profile__section choose-profile__section--wled">
                            <h4 class="choose-profile__section-title">WLED:</h4>
                            <p v-if="!wled" class="choose-profile__value choose-profile__value--muted">Not set</p>
                            <template v-else>
                                <p v-if="wled.on === false" class="choose-profile__value">Status: Turned Off</p>
                                <p v-else-if="wled.lor === 0" class="choose-profile__value">Status: Ambilight</p>
                                <p v-else-if="wled.preset_name" class="choose-profile__value">Preset: {{ wled.preset_name }}</p>
                                <p v-else-if="wled.ps != null" class="choose-profile__value">Preset ID: {{ wled.ps }}</p>
                                <p v-else class="choose-profile__value">Status: Turned Off</p>
                            </template>
                        </section>

                        <!-- Lights Section -->
                        <section class="choose-profile__section choose-profile__section--lights">
                            <h4 class="choose-profile__section-title">Main lights:</h4>
                            <p v-if="lights" class="choose-profile__value">{{ lights.payload.state ? 'Włączone' : 'Wyłączone' }}</p>
                            <p v-else class="choose-profile__value choose-profile__value--muted">Not Set</p>
                        </section>

                        <!-- AC Section -->
                        <section class="choose-profile__section choose-profile__section--ac">
                            <h4 class="choose-profile__section-title">Air Conditioning:</h4>
                            <p v-if="ac" class="choose-profile__value">Set Temperature: {{ ac.payload.requestedTemp }}°C</p>
                            <p v-else class="choose-profile__value choose-profile__value--muted">Not set</p>
                        </section>

                        <!-- Blinds Section -->
                        <section class="choose-profile__section choose-profile__section--blinds">
                            <h4 class="choose-profile__section-title">Window Blinds:</h4>
                            <p v-if="blinds?.state === 'open'" class="choose-profile__value">Status: Open</p>
                            <p v-else-if="blinds?.state === 'close'" class="choose-profile__value">Status: Closed</p>
                            <p v-else-if="blinds?.automate" class="choose-profile__value">
                                Status: Automatic (Min: {{ blinds.minLux }}, Max: {{ blinds.maxLux }})
                            </p>
                            <p v-else class="choose-profile__value choose-profile__value--muted">Not set</p>
                        </section>

                        <!-- Action buttons -->
                        <div class="choose-profile__actions">
                            <button class="btn choose-profile__btn choose-profile__btn--apply" @click="applyProfile">Use profile</button>
                            <button class="btn choose-profile__btn choose-profile__btn--delete" @click="promptDeleteProfile">Delete profile</button>
                        </div>
                    </div>
                </div>

            </div>
        </div>
    </div>
    <dialog-box ref="dialogRef" />
</template>

<script setup>
// Component: Choose Profile
// Responsibilities:
// 1. Fetch and list available profiles
// 2. Show configuration details of the selected profile
// 3. Apply or delete a profile (dispatching WS messages & HTTP calls)
// 4. Refresh if parent notifies name changes via prop

import { ref, computed, onMounted, watch } from 'vue';
import { useAutomateStore } from '@/stores/automateStore';
import { useLinkStore } from '@/stores/linkStore';
import { useWsStore } from '@/stores/wsStore';
import dialogBox from '@/components/dialogBox_component.vue'

// --- Stores ---
const automateStore = useAutomateStore();
const linkStore = useLinkStore();
const wsStore = useWsStore();

// --- Props & Emits ---
const props = defineProps({
    // Set of names kept outside so sibling components can react on profile create/delete
    sharedProfileNames: { type: Set, default: () => new Set() }
});
const emit = defineEmits(['update-profiles']);

// --- Reactive State ---
const profiles = ref([]);            // Loaded profiles array
const loading = ref(true);           // Loading flag
const error = ref(null);             // Error message or null
const selectedProfileId = ref('');   // Currently selected profile id
const dialogRef = ref(null);

// Derived currently selected full profile object (or null)
const selectedProfile = computed(() =>
    profiles.value.find(p => p.profile_id === selectedProfileId.value) || null
);

// Section-level computed shortcuts
const wled   = computed(() => selectedProfile.value?.profile_json?.WLED);
const lights = computed(() => selectedProfile.value?.profile_json?.lights);
const ac     = computed(() => selectedProfile.value?.profile_json?.AC);
const blinds = computed(() => selectedProfile.value?.profile_json?.blinds);

// --- WebSocket helper ---
// Opens a short-lived connection just to send a single payload, then closes.
function sendViaWebSocket(payload) {
    const ws = new WebSocket(wsStore.wsUrl);
    ws.onopen = () => ws.send(JSON.stringify(payload));
    // Ensure closure regardless of outcome to avoid accumulating sockets
    ws.onmessage = ws.onerror = ws.onclose = () => ws.close();
}

// --- Fetch profiles list from backend ---
async function fetchProfiles() {
    loading.value = true;
    error.value = null;
    try {
        const response = await fetch(linkStore.getPhpApiUrl('chooseProfile.php'));
        if (!response.ok) throw new Error(`HTTP error! Status: ${response.status}`);
        const data = await response.json();
        if (data.success) {
            profiles.value = data.profiles;
            // Emit updated name set to parent (helps syncing other components)
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

// --- Apply currently selected profile ---
async function applyProfile() {
    if (!selectedProfile.value) return;
    const profile = selectedProfile.value.profile_json;

    // Handlers map: each key corresponds to a subsystem in stored JSON
    const handlers = {
        WLED:  (w) => linkStore.sendWledCommand(w),
        lights:(l) => sendViaWebSocket({ channel: l.channel, lightON: l.payload.state }),
        AC:    (a) => sendViaWebSocket(a),
        blinds:(b) => applyBlindsPayload(b)
    };

    for (const [key, fn] of Object.entries(handlers)) {
        if (profile?.[key]) await fn(profile[key]);
    }

    dialogRef.value?.openMessage('Profile has been applied successfully');
}

// --- Delete selected profile ---
async function deleteProfile() {
    if (!selectedProfile.value) return;
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
            dialogRef.value?.openMessage(`Profile ${data.deleted_name || ''} has been deleted succesfuly`);
        } else {
            throw new Error(data.error || 'Profile could not be deleted');
        }
    } catch (err) {
        dialogRef.value?.openMessage(`Error deleting profile: ${err.message}`);
    }
}

async function promptDeleteProfile() {
    if (!selectedProfile.value) return;
    const name = selectedProfile.value.profile_name;
    const confirmed = await dialogRef.value?.openConfirm(`Are you sure you want to delete profile ${name}?`);
    if (confirmed) {
        await deleteProfile();
    }
}

// --- Apply blinds payload branch: command or automation config ---
async function applyBlindsPayload(payload) {
    if (typeof payload.state === 'string') {
        // Manual open/close path: disable automation first
        automateStore.automate_flag = false;
        await fetch(linkStore.getPhpApiUrl('setBlindsAutomate.php'), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ automate: false })
        });
        await fetch(linkStore.getPhpApiUrl('blindsControl.php') + `?action=${payload.state}`);
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

// --- Watch for external profile name changes ---
watch(() => props.sharedProfileNames, async (newNames) => {
    const currentNames = new Set(profiles.value.map(p => p.profile_name));
    const needsRefresh = [...newNames].some(name => !currentNames.has(name));
    if (needsRefresh) await fetchProfiles();
}, { deep: true });
</script>

<style lang="scss" scoped>
// Local block: choose-profile
// Do not modify global .card styles; only extend within block scope

$cp-gap: 1.5rem;
$cp-border-radius: 8px;
$cp-panel-bg: rgba(255,255,255,0.05);
$cp-border-color: #ddd;
$cp-muted: rgba(255,255,255,0.55);
$cp-error: #d9534f;
$cp-transition: 0.2s ease;

.choose-profile {
  &__content { display:flex; flex-direction:column; gap:$cp-gap; }

  /* State messages */
  &__state { padding:1.5rem; text-align:center; font-size:0.95rem; }
  &__state--error { color:$cp-error; }
  &__state--empty { color:$cp-muted; }
  &__state--loading { color:$cp-muted; font-style:italic; }

  /* Selector region */
  &__selector { display:flex; flex-direction:column; gap:$cp-gap; }
  &__dropdown { display:flex; flex-direction:column; gap:0.5rem; }
  &__label { font-weight:600; font-size:1.05rem; }
  &__select {
    padding:0.75rem; font-size:1rem; border-radius:4px; border:1px solid #ccc;
    background:var(--color-background); color:var(--color-text);
    transition:border-color $cp-transition;
    &:focus { outline:none; border-color: var(--color-accent,#4CAF50); }
  }

  /* Details panel */
  &__details {
    margin-top:0.25rem; padding:1rem; border:1px solid $cp-border-color; border-radius:$cp-border-radius;
    background:$cp-panel-bg; backdrop-filter: blur(2px);
  }
  &__details-title { margin:0 0 1rem; font-size:1.15rem; font-weight:600; color:var(--color-heading); }

  /* Section blocks */
  &__section { margin-bottom:1rem; padding-bottom:0.75rem; border-bottom:1px solid rgba(255,255,255,0.1); }
  &__section:last-child { border-bottom:none; }
  &__section-title { margin:0.4rem 0; font-size:0.95rem; font-weight:600; color:var(--color-heading); }

  &__value { margin:0.25rem 0; font-size:0.85rem; line-height:1.3; }
  &__value--muted { color:$cp-muted; }

  /* Actions */
  &__actions { display:flex; justify-content:space-between; gap:1rem; margin-top:1.25rem; }
  &__btn { flex:1; padding:0.65rem 1rem; font-size:0.85rem; border:none; border-radius:4px; cursor:pointer; transition:filter $cp-transition, transform $cp-transition; }
  &__btn--apply { background:#4CAF50; color:#fff; }
  &__btn--delete { background:#f44336; color:#fff; }
  &__btn:hover { filter:brightness(1.1); }
  &__btn:active { transform:translateY(1px); }
}
</style>
