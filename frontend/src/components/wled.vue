<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Sterowanie WLED</h2>
        </div>
        <div class="card__body">
            <div class="card__content">
                <div class="button-group">
                    <button class="btn" @click="wledON" :disabled="loading">
                        {{ 'Włącz WLED' }}
                    </button>

                    <button class="btn" @click="wledOFF" :disabled="loading">
                        {{ 'Wyłącz WLED' }}
                    </button>

                    <button class="btn" @click="ambilightON" :disabled="loading">
                        {{ 'Włącz Ambilight' }}
                    </button>
                    <button class="btn" @click="ambilightOFF" :disabled="loading">
                        {{ 'Wyłącz Ambilight' }}
                    </button>
                </div>
            </div>
        </div>
        <div class="card__footer" v-if="lightInfo">
            <div class="card__info-item">
                <span class="card__label">Zużycie prądu:</span>
                <span class="card__value">{{ lightInfo.power || 'N/A' }}</span>
            </div>
            <div class="card__info-item">
                <span class="card__label">Status:</span>
                <span class="card__value">{{ lightInfo.status || 'N/A' }}</span>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'


</script>

<style lang="scss" scoped></style>