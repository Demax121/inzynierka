<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Wybierz Profil</h2>
        </div>
        <div class="card__body">
            <div class="card__content">
                <!-- Profile Selection Dropdown -->
                <div class="form-group">
                    <label for="profileSelect" class="form-label">Dostępne profile:</label>
                    <select 
                        id="profileSelect" 
                        v-model="selectedProfileId" 
                        class="form-select"
                        @change="onProfileChange"
                    >
                        <option value="">-- Wybierz profil --</option>
                        <option 
                            v-for="profile in profiles" 
                            :key="profile.id" 
                            :value="profile.id"
                        >
                            {{ profile.name }}
                        </option>
                    </select>
                </div>

                <!-- Profile Information Display -->
                <div class="profile-info" v-if="selectedProfile">
                    <h3 class="profile-info__title">Szczegóły profilu: {{ selectedProfile.name }}</h3>
                    
                    <div class="profile-info__grid">
                        <!-- Rolety -->
                        <div class="info-item">
                            <div class="info-item__label">Rolety:</div>
                            <div class="info-item__value">
                                <span class="status-badge" :class="getBlindsStatusClass(selectedProfile.blinds)">
                                    {{ formatBlindsStatus(selectedProfile.blinds) }}
                                </span>
                            </div>
                        </div>

                        <!-- Główne światło -->
                        <div class="info-item">
                            <div class="info-item__label">Główne światło:</div>
                            <div class="info-item__value">
                                <span class="status-badge" :class="getLightStatusClass(selectedProfile.mainLight)">
                                    {{ formatLightStatus(selectedProfile.mainLight) }}
                                </span>
                            </div>
                        </div>

                        <!-- LED -->
                        <div class="info-item">
                            <div class="info-item__label">LED:</div>
                            <div class="info-item__value">
                                <span class="status-badge status-badge--led">
                                    {{ selectedProfile.led }}
                                </span>
                            </div>
                        </div>

                        <!-- Kolor LED (tylko dla WLED) -->
                        <div class="info-item" v-if="selectedProfile.led === 'WLED' && selectedProfile.ledColor">
                            <div class="info-item__label">Kolor LED:</div>
                            <div class="info-item__value">
                                <div class="color-display">
                                    <div 
                                        class="color-preview" 
                                        :style="{ backgroundColor: selectedProfile.ledColor }"
                                    ></div>
                                    <span class="color-code">{{ selectedProfile.ledColor }}</span>
                                </div>
                            </div>
                        </div>

                        <!-- Sterowanie AC -->
                        <div class="info-item" v-if="selectedProfile.ac">
                            <div class="info-item__label">Temperatura AC:</div>
                            <div class="info-item__value">
                                <span>{{ selectedProfile.ac }}°C</span>
                            </div>
                        </div>
                    </div>

                    <!-- Apply Profile Button -->
                    <div class="profile-actions">
                        <button 
                            class="btn btn--primary" 
                            @click="applyProfile"
                            :disabled="loading"
                        >
                            {{ loading ? 'Stosowanie...' : 'Zastosuj profil' }}
                        </button>
                    </div>
                </div>

                <!-- No Profile Selected Message -->
                <div class="no-profile" v-else-if="selectedProfileId === ''">
                    <div class="no-profile__icon">📋</div>
                    <p class="no-profile__text">Wybierz profil z listy powyżej, aby zobaczyć jego szczegóły</p>
                </div>

                <!-- No Profiles Available Message -->
                <div class="no-profiles" v-if="profiles.length === 0">
                    <div class="no-profiles__icon">⚠️</div>
                    <p class="no-profiles__text">Brak dostępnych profili. Utwórz nowy profil w konfiguratorze.</p>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'

// Reactive data
const selectedProfileId = ref('')
const loading = ref(false)

// Sample profiles data (replace with actual API call later)
const profiles = ref([
    {
        id: 1,
        name: 'Wieczorny relaks',
        blinds: 'zamkniete',
        mainLight: 'wylaczone',
        led: 'WLED',
        ledColor: '#ff6b35',
        ac: 22.5
    },
    {
        id: 2,
        name: 'Poranne czytanie',
        blinds: 'otwarte',
        mainLight: 'wlaczone',
        led: 'WLED',
        ledColor: '#ffffff',
        ac: 21.0
    },
    {
        id: 3,
        name: 'Film w domu',
        blinds: 'zamkniete',
        mainLight: 'wylaczone',
        led: 'ambilight',
        ledColor: null,
        ac: 23.0
    },
    {
        id: 4,
        name: 'Tryb pracy',
        blinds: 'otwarte',
        mainLight: 'wlaczone',
        led: 'WLED',
        ledColor: '#4a90e2',
        ac: 20.5
    }
])

// Computed property to get selected profile
const selectedProfile = computed(() => {
    return profiles.value.find(profile => profile.id === selectedProfileId.value) || null
})

// Event handlers
const onProfileChange = () => {
    console.log('Selected profile:', selectedProfile.value)
}

const applyProfile = async () => {
    if (!selectedProfile.value) return
    
    loading.value = true
    try {
        // Here you will add the actual functionality to apply the profile
        console.log('Applying profile:', selectedProfile.value)
        
        // Example: Send profile data to your backend/devices
        // await applyProfileToDevices(selectedProfile.value)
        
        // Simulate API delay
        await new Promise(resolve => setTimeout(resolve, 1500))
        
        alert(`Profil "${selectedProfile.value.name}" został zastosowany pomyślnie!`)
    } catch (error) {
        console.error('Error applying profile:', error)
        alert('Błąd podczas stosowania profilu')
    } finally {
        loading.value = false
    }
}

// Formatting functions
const formatBlindsStatus = (status) => {
    return status === 'otwarte' ? 'Otwarte' : 'Zamknięte'
}

const formatLightStatus = (status) => {
    return status === 'wlaczone' ? 'Włączone' : 'Wyłączone'
}

// CSS class helpers
const getBlindsStatusClass = (status) => {
    return status === 'otwarte' ? 'status-badge--success' : 'status-badge--warning'
}

const getLightStatusClass = (status) => {
    return status === 'wlaczone' ? 'status-badge--success' : 'status-badge--error'
}

// Load profiles on component mount 
onMounted(async () => {
    // Here you will load profiles from your backend
    // try {
    //     const response = await fetch('/api/getProfiles.php')
    //     profiles.value = await response.json()
    // } catch (error) {
    //     console.error('Error loading profiles:', error)
    // }
})

</script>

<style lang="scss" scoped>
.form {
    &-group {
        margin-bottom: 2rem;
    }

    &-label {
        display: block;
        font-weight: 600;
        margin-bottom: 0.5rem;
        color: var(--text-primary, #333);
        font-size: 1rem;
    }

    &-select {
        width: 100%;
        padding: 0.75rem 1rem;
        border: 2px solid var(--border-color, #e1e5e9);
        border-radius: 8px;
        font-size: 1rem;
        transition: border-color 0.2s ease, box-shadow 0.2s ease;
        background-color: var(--input-bg, #fff);
        cursor: pointer;
        background-image: url("data:image/svg+xml,%3csvg xmlns='http://www.w3.org/2000/svg' fill='none' viewBox='0 0 20 20'%3e%3cpath stroke='%236b7280' stroke-linecap='round' stroke-linejoin='round' stroke-width='1.5' d='m6 8 4 4 4-4'/%3e%3c/svg%3e");
        background-position: right 0.75rem center;
        background-repeat: no-repeat;
        background-size: 1.5em 1.5em;
        padding-right: 2.5rem;

        &:focus {
            outline: none;
            border-color: var(--primary-color, #007bff);
            box-shadow: 0 0 0 3px rgba(0, 123, 255, 0.1);
        }
    }
}

.profile {
    &-info {
        background-color: var(--bg-light, #f8f9fa);
        border-radius: 12px;
        padding: 1.5rem;
        margin-top: 1.5rem;
        border: 1px solid var(--border-color, #e1e5e9);

        &__title {
            margin: 0 0 1.5rem 0;
            color: var(--text-primary, #333);
            font-size: 1.25rem;
            font-weight: 600;
        }

        &__grid {
            display: grid;
            gap: 1rem;
            margin-bottom: 1.5rem;
        }
    }

    &-actions {
        display: flex;
        justify-content: center;
        padding-top: 1rem;
        border-top: 1px solid var(--border-light, #f0f0f0);
    }
}

.info {
    &-item {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 0.75rem;
        background-color: white;
        border-radius: 8px;
        border: 1px solid var(--border-light, #f0f0f0);

        &__label {
            font-weight: 500;
            color: var(--text-secondary, #666);
            font-size: 0.95rem;
        }

        &__value {
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }
    }
}

.color {
    &-display {
        display: flex;
        align-items: center;
        gap: 0.5rem;
    }

    &-preview {
        width: 24px;
        height: 24px;
        border-radius: 50%;
        border: 2px solid white;
        box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.1);
    }

    &-code {
        font-family: monospace;
        font-size: 0.9rem;
        color: var(--text-secondary, #666);
        background-color: var(--bg-code, #f1f3f4);
        padding: 0.2rem 0.4rem;
        border-radius: 4px;
    }
}

.no {
    &-profile,
    &-profiles {
        text-align: center;
        padding: 3rem 1rem;
        color: var(--text-secondary, #666);

        &__icon {
            font-size: 3rem;
            margin-bottom: 1rem;
        }

        &__text {
            font-size: 1.1rem;
            margin: 0;
            line-height: 1.5;
        }
    }

    &-profiles {
        background-color: var(--bg-light, #f8f9fa);
        border-radius: 12px;
        margin-top: 1.5rem;
        border: 1px solid var(--border-color, #e1e5e9);
    }
}

// Responsive design
@media (max-width: 768px) {
    .info {
        &-item {
            flex-direction: column;
            align-items: flex-start;
            gap: 0.5rem;

            &__value {
                width: 100%;
                justify-content: flex-start;
            }
        }
    }

    .profile {
        &-info {
            padding: 1rem;
        }
    }

    .btn {
        width: 100%;
    }
}
</style>