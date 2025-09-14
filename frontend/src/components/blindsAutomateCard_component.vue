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

                    <!-- added checkbox for "automat" -->
                    <div class="input-group">
                        <label for="automat">automat</label>
                        <input id="automat" type="checkbox" v-model="automate" />
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
import { ref, onMounted } from 'vue'
import { useLinkStore } from '@/stores/linkStore'


const status = ref('')
const loading = ref(false)
const currentAction = ref('')
const minLux = ref(0)
const maxLux = ref(0)
const automate = ref(false);


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
            status.value = 'Konfiguracja załadowana.'
            console.log(`Min lux: ${minLux.value}, Max lux: ${maxLux.value}`)
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

onMounted(() => {
    getLuxConfig()
})


const saveLuxConfig = async () => {
    status.value = 'Zapisano'
    if (minLux.value >= maxLux.value) {
        status.value = 'Błąd: Min lux musi być mniejsze niż Max lux.'
        return
    }
    const res = await fetch(linkStore.getPhpApiUrl('saveBlindsConfig.php'), {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ minLux: minLux.value, maxLux: maxLux.value })
    })
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
