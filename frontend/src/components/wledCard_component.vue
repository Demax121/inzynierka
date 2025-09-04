<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Sterowanie WLED</h2>
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
                        {{ 'Tryb WLED' }}
                    </button>

                    <button class="btn" @click="sendCommand({ on: true, lor: 0 })" :disabled="loading">
                        {{ 'Tryb Ambilight' }}
                    </button>

                    <button class="btn" @click="sendCommand({ on: false })" :disabled="loading">
                        {{ 'Wyłącz LED' }}
                    </button>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'

const WLED_IP = "http://192.168.1.25"; // Adres WLED
const WLED_ENDPOINT = "/json/state";

function sendCommand(payload) {
    fetch(WLED_IP + WLED_ENDPOINT, {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify(payload)
    })
        .then(response => response.json())
        .then(data => console.log("Odpowiedź WLED:", data))
        .catch(error => console.error("Błąd:", error));
}



</script>

<style lang="scss" scoped></style>