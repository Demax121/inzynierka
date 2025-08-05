<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Klimatyzacja</h2>
        </div>
        <div class="card__body">
            <div class="card__content">
                <!-- Temperature Setting Form -->
                <div class="temperature-form">
                    <form class="temp-form">
                        <input 
                            type="number" 
                            class="temp-input"
                            min="16"
                            max="30"
                            placeholder="22"
                        />
                        <button type="submit" class="btn btn--temperature">
                            Ustaw temperaturę
                        </button>
                    </form>
                </div>

                <!-- AC Control Buttons -->
                <div class="button-group">
                    <button class="btn">Włącz Klimatyzację</button>
                    <button class="btn">Wyłącz Klimatyzację</button>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
// Simplified component - no JavaScript functionality for now
</script>

<style lang="scss" scoped>
.temperature-form {
    background: #f8f9fa;
    border-radius: 8px;
    padding: 1rem;
    margin-bottom: 1rem;
}

.btn--temperature{
    max-width: 20ch;
}

.temp-form {
    display: flex;
    gap: 0.5rem;
    align-items: center;
}

.temp-input {
    padding: 0.5rem;
    border: 1px solid #ccc;
    border-radius: 4px;
    width: 10rem;
    min-width: 5rem;
    font-size: 16pt;
}

@media (max-width: 768px) {
    .button-group {
        flex-direction: column;
    }
}
</style>