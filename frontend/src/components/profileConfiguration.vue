<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Konfigurator Profili</h2>
        </div>
        <div class="card__body">
            <div class="card__content">
                <form class="profile-form">
                    <!-- Nazwa profilu -->
                    <div class="form-group">
                        <label for="profileName" class="form-label">Nazwa profilu:</label>
                        <input 
                            type="text" 
                            id="profileName" 
                            class="form-input"
                            placeholder="Wprowadź nazwę profilu"
                            required
                        />
                    </div>

                    <!-- Rolety -->
                    <div class="form-group">
                        <label for="blinds" class="form-label">Rolety:</label>
                        <select 
                            id="blinds" 
                            class="form-select"
                            required
                        >
                            <option value="">-- Wybierz stan rolet --</option>
                            <option value="otwarte">Otwarte</option>
                            <option value="zamkniete">Zamknięte</option>
                        </select>
                    </div>

                    <!-- Główne światło -->
                    <div class="form-group">
                        <label for="mainLight" class="form-label">Główne światło:</label>
                        <select 
                            id="mainLight" 
                            class="form-select"
                            required
                        >
                            <option value="">-- Wybierz stan światła --</option>
                            <option value="wlaczone">Włączone</option>
                            <option value="wylaczone">Wyłączone</option>
                        </select>
                    </div>

                    <!-- LED -->
                    <div class="form-group">
                        <label for="led" class="form-label">LED:</label>
                        <select 
                            id="led" 
                            class="form-select"
                            required
                        >
                            <option value="">-- Wybierz tryb LED --</option>
                            <option value="WLED">WLED</option>
                            <option value="ambilight">Ambilight</option>
                        </select>
                    </div>

                    <!-- Kolor LED -->
                    <div class="form-group">
                        <label for="ledColor" class="form-label">Kolor LED:</label>
                        <div class="color-picker-container">
                            <input 
                                type="color" 
                                id="ledColor" 
                                class="form-color-picker"
                                value="#ff0000"
                            />
                            <span class="color-value">#ff0000</span>
                        </div>
                    </div>

                    <!-- Sterowanie AC -->
                    <div class="form-group">
                        <label for="ac" class="form-label">Sterowanie AC (temperatura °C):</label>
                        <input 
                            type="number" 
                            id="ac" 
                            class="form-input"
                            min="16"
                            max="30"
                            step="0.5"
                            placeholder="np. 22.5"
                        />
                        <small class="form-hint">Zakres: 16-30°C</small>
                    </div>

                    <!-- Przyciski -->
                    <div class="form-actions">
                        <button type="submit" class="btn">
                            Zapisz profil
                        </button>
                        <button type="button" class="btn">
                            Wyczyść formularz
                        </button>
                    </div>
                </form>
            </div>
        </div>
    </div>
</template>

<script setup>
// Simplified component - no JavaScript functionality for now
</script>

<style lang="scss" scoped>
.profile-form {
  max-width: 600px;
  margin-bottom: 2rem;
}

.form-group {
  margin-bottom: 1.5rem;
}

.form-label {
  display: block;
  font-weight: 600;
  margin-bottom: 0.5rem;
  color: var(--text-primary, #333);
  font-size: 0.95rem;
}

.form-input,
.form-select {
  width: 100%;
  padding: 0.75rem 1rem;
  border: 2px solid var(--border-color, #e1e5e9);
  border-radius: 8px;
  font-size: 1rem;
  transition: border-color 0.2s ease, box-shadow 0.2s ease;
  background-color: var(--input-bg, #fff);

  &:focus {
    outline: none;
    border-color: var(--primary-color, #007bff);
    box-shadow: 0 0 0 3px rgba(0, 123, 255, 0.1);
  }

  &:disabled {
    background-color: var(--disabled-bg, #f8f9fa);
    color: var(--disabled-text, #6c757d);
    cursor: not-allowed;
  }
}

.form-select {
  cursor: pointer;
  background-image: url("data:image/svg+xml,%3csvg xmlns='http://www.w3.org/2000/svg' fill='none' viewBox='0 0 20 20'%3e%3cpath stroke='%236b7280' stroke-linecap='round' stroke-linejoin='round' stroke-width='1.5' d='m6 8 4 4 4-4'/%3e%3c/svg%3e");
  background-position: right 0.75rem center;
  background-repeat: no-repeat;
  background-size: 1.5em 1.5em;
  padding-right: 2.5rem;
}

.color-picker-container {
  display: flex;
  align-items: center;
  gap: 1rem;
}

.form-color-picker {
  width: 60px;
  height: 40px;
  padding: 0;
  border: 2px solid var(--border-color, #e1e5e9);
  border-radius: 8px;
  cursor: pointer;
  background: none;

  &::-webkit-color-swatch-wrapper {
    padding: 0;
  }

  &::-webkit-color-swatch {
    border: none;
    border-radius: 6px;
  }

  &::-moz-color-swatch {
    border: none;
    border-radius: 6px;
  }
}

.color-value {
  font-family: monospace;
  font-size: 0.9rem;
  color: var(--text-secondary, #666);
  background-color: var(--bg-light, #f8f9fa);
  padding: 0.25rem 0.5rem;
  border-radius: 4px;
}

.form-hint {
  display: block;
  margin-top: 0.25rem;
  font-size: 0.85rem;
  color: var(--text-secondary, #666);
}

.form-actions {
  display: flex;
  gap: 1rem;
  margin-top: 2rem;
}

.btn {
  padding: 0.75rem 1.5rem;
  border: none;
  border-radius: 8px;
  font-size: 1rem;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
  text-decoration: none;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 120px;

  &:disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }

  &--primary {
    background-color: var(--primary-color, #007bff);
    color: white;

    &:hover:not(:disabled) {
      background-color: var(--primary-hover, #0056b3);
      transform: translateY(-1px);
    }
  }

  &--secondary {
    background-color: var(--secondary-color, #6c757d);
    color: white;

    &:hover:not(:disabled) {
      background-color: var(--secondary-hover, #545b62);
      transform: translateY(-1px);
    }
  }
}

.button-group {
  margin-top: 2rem;
  padding-top: 2rem;
  border-top: 1px solid var(--border-color, #e1e5e9);
}

// Responsive design
@media (max-width: 768px) {
  .form-actions {
    flex-direction: column;
  }

  .color-picker-container {
    flex-direction: column;
    align-items: flex-start;
  }

  .profile-form {
    max-width: 100%;
  }
}
</style>