<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Automatyka rolet</h2>
        </div>
        <div class="card__body">
            <div class="card__content">
                <div class="status" v-if="status">
                    <p class="card__text card__text--bold">{{ status }}</p>
                </div>

                <div class="lux-config-form">
                    <div class="input-group">
                        <label for="minLux">Min lux:</label>
                        <input id="minLux" v-model.number="minLux" type="number" class="lux-input" />
                    </div>
                    <div class="input-group">
                        <label for="maxLux">Max lux:</label>
                        <input id="maxLux" v-model.number="maxLux" type="number" class="lux-input" />
                    </div>

                    <!-- zmieniony checkbox na slider -->
                    <div class="input-group input-group--slider">
                        <span class="slider-label">Tryb automatyczny:</span>
                        <label class="switch switch--small">
                            <input type="checkbox" v-model="automate" @change="handleAutomateChange">
                            <span class="slider"></span>
                        </label>
                    </div>
                </div>

                <div class="button-group">
                    <button class="btn" @click="saveLuxConfig" :disabled="loading">
                        {{ loading && currentAction === 'save' ? 'Zapisywanie...' : 'Ustaw granice' }}
                    </button>

                    
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import { useLinkStore } from '@/stores/linkStore'

// Define props to receive handlers from parent
const props = defineProps({
  openBlindsHandler: {
    type: Function,
    required: true
  },
  closeBlindsHandler: {
    type: Function,
    required: true
  }
});

const status = ref('')
const loading = ref(false)
const currentAction = ref('')
const minLux = ref(0)
const maxLux = ref(0)
const automate = ref(false);

let ws;

const linkStore = useLinkStore()


const getLuxConfig = async () => {
    loading.value = true
    status.value = 'Ładowanie konfiguracji...'
    try {
        const res = await fetch(linkStore.getPhpApiUrl('getBlindsConfig.php'))
        const data = await res.json()


        if (data && typeof data === 'object') {
            if (data.min_lux !== undefined) minLux.value = Number(data.min_lux)
            if (data.max_lux !== undefined) maxLux.value = Number(data.max_lux)
            if (data.automate !== undefined) automate.value = Boolean(data.automate)
            status.value = 'Konfiguracja załadowana.'
        } else {
            status.value = 'Błąd: Nieprawidłowy format odpowiedzi.'
        }
    } catch (error) {
        console.error('Error fetching lux config:', error)
        status.value = `Błąd połączenia: ${error.message}`
    } finally {
        loading.value = false
    }
}

// Use the handlers from props instead of implementing our own functions
// Handler for when the automate toggle changes
const handleAutomateChange = () => {
    // Save the automate state immediately when it changes
    saveLuxConfigQuiet();
    
    // Update the last change timestamp to prevent immediate automation
    lastAutomateChange.value = Date.now();
    
    if (automate.value) {
        status.value = 'Automatyka włączona';
    } else {
        status.value = 'Automatyka wyłączona';
    }
    
    // Clear status after a delay
    setTimeout(() => {
        if (status.value === 'Automatyka włączona' || status.value === 'Automatyka wyłączona') {
            status.value = '';
        }
    }, 3000);
};

const openBlinds = () => {
    status.value = 'Otwieranie rolet...'
    props.openBlindsHandler();
    // Set status back after a delay to simulate completion
    setTimeout(() => {
        status.value = 'Rolety otwarte';
    }, 2000);
}

const closeBlinds = () => {
    status.value = 'Zamykanie rolet...'
    props.closeBlindsHandler();
    // Set status back after a delay to simulate completion
    setTimeout(() => {
        status.value = 'Rolety zamknięte';
    }, 2000);
}

// Automatically control blinds based on lux value and configured thresholds
const handleLuxAutomation = (luxValue) => {
    if (!automate.value) {
        return; // Automation disabled
    }
    
    if (luxValue !== null) {
        if (luxValue < minLux.value) {
            // Too dark, close blinds
            closeBlinds();
            console.log(`Auto-closing blinds: lux (${luxValue}) < min_lux (${minLux.value})`);
        } else if (luxValue >= minLux.value && luxValue < maxLux.value) {
            // Ideal range, open blinds
            openBlinds();
            console.log(`Auto-opening blinds: min_lux (${minLux.value}) <= lux (${luxValue}) < max_lux (${maxLux.value})`);
        } else if (luxValue >= maxLux.value) {
            // Too bright, close blinds
            closeBlinds();
            console.log(`Auto-closing blinds: lux (${luxValue}) >= max_lux (${maxLux.value})`);
        }
    }
}

// Keep track of the last time the automation state was changed
const lastAutomateChange = ref(Date.now());
// Debounce time to prevent automation running too soon after toggling (in milliseconds)
const AUTOMATE_DEBOUNCE = 2000;

onMounted(() => {
    getLuxConfig();
    
    // Setup WebSocket connection
    ws = new WebSocket('ws://192.168.1.4:8886');
    
    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            if (data.channel === 'lux_sensor' && data.lux !== undefined) {
                // Only process automation if we're not in the debounce period after toggling automate
                if (Date.now() - lastAutomateChange.value > AUTOMATE_DEBOUNCE) {
                    handleLuxAutomation(data.lux);
                }
            }
        } catch (error) {
            console.error('WebSocket message error:', error);
        }
    };
    
    ws.onopen = () => {
        console.log('WebSocket connected for blinds automation');
    };
    
    ws.onclose = () => {
        console.log('WebSocket connection closed for blinds automation');
    };
    
    ws.onerror = (error) => {
        console.error('WebSocket error for blinds automation:', error);
    };
})

onUnmounted(() => {
    if (ws) ws.close();
})


// Function to disable automation when manual control is used
const disableAutomation = () => {
    if (automate.value === true) {
        automate.value = false;
        // Save the new state to the server to persist it
        saveLuxConfigQuiet();
        // Update the last change timestamp
        lastAutomateChange.value = Date.now();
        status.value = 'Automatyka wyłączona (sterowanie ręczne)';
        setTimeout(() => {
            if (status.value === 'Automatyka wyłączona (sterowanie ręczne)') {
                status.value = '';
            }
        }, 3000);
    }
};

// Expose the disableAutomation method to parent component
defineExpose({
    disableAutomation
});

// Save config without showing status messages (used by disableAutomation)
const saveLuxConfigQuiet = async () => {
    try {
        await fetch(linkStore.getPhpApiUrl('saveBlindsConfig.php'), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ 
                minLux: minLux.value, 
                maxLux: maxLux.value,
                automate: automate.value 
            })
        });
    } catch (error) {
        console.error('Error saving config quietly:', error);
    }
};

const saveLuxConfig = async () => {
    loading.value = true
    currentAction.value = 'save'
    status.value = 'Zapisywanie...'
    
    if (minLux.value >= maxLux.value) {
        status.value = 'Błąd: Min lux musi być mniejsze niż Max lux.'
        loading.value = false
        return
    }
    
    try {
        const res = await fetch(linkStore.getPhpApiUrl('saveBlindsConfig.php'), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ 
                minLux: minLux.value, 
                maxLux: maxLux.value,
                automate: automate.value 
            })
        })
        
        const data = await res.json()
        if (data && data.success) {
            status.value = 'Zapisano pomyślnie'
        } else {
            status.value = 'Błąd podczas zapisywania'
        }
    } catch (error) {
        console.error('Error saving config:', error)
        status.value = `Błąd połączenia: ${error.message}`
    } finally {
        loading.value = false
        currentAction.value = ''
    }
}
</script>

<style lang="scss" scoped>
$input-width: 120px;
$input-padding: 6px 10px;
$input-font-size: 11pt;
$form-margin-bottom: 0rem;
$input-group-gap: 0.4rem;
$button-padding: 11px 15px;
$button-font-size: 12pt;

.lux-config-form {
    margin-bottom: $form-margin-bottom;
    padding: 1rem;
    border: 1px solid #ddd;
    border-radius: 8px;
    background-color: #f9f9f9;
    text-align: center;
}

.input-group {
    display: flex;
    align-items: center;
    margin-bottom: $input-group-gap;
    gap: 1rem;
    justify-content: center;
    
    &--slider {
        margin: 1rem 0;
    }
}

.slider-label {
    min-width: 70px;
    font-weight: bold;
    font-size: 14px;
}

.input-group label {
    min-width: 70px;
    font-weight: bold;
    font-size: 14px;
}

.lux-input {
    padding: $input-padding;
    font-size: $input-font-size;
    width: $input-width;
    border: 2px solid #ccc;
    border-radius: 4px;
    outline: none;
    transition: border-color 0.3s;
}

.lux-input:focus {
    border-color: var(--color, #007bff);
}

.button-group {
    margin-top: 0.5rem;
}

.device-info {
    border-radius: 10px;
    filter: drop-shadow(0 5px 10px 0 #ffffff);
    width: 15rem;
    height: 5rem;
    background-color: $background-crl-secondary;
    padding: 1rem;
    color: white;
    font-family: "Poppins", sans-serif;
}
</style>
