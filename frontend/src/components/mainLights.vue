<template>
    <div class="card-container">
        <h2 class="card__title">Sterowanie głównym oświetleniem</h2>

        <div class="button-group">
            <button class="btn" @click="LightsON" :disabled="loading">
                {{ 'Włącz światło' }}
            </button>

            <button class="btn" @click="LightsOFF" :disabled="loading">
                {{ 'Wyłącz światło' }}
            </button>

            <button class="btn" @click="getLightsStatus" :disabled="loading">
                {{ 'Sprawdź status' }}
            </button>
        </div>
        <div class="device-info" v-if="lightInfo">
            <p>Zużycie prądu: {{}}</p>
            <p>Status: {{}}</p>
        </div>
    </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'


</script>

<style lang="scss" scoped></style>