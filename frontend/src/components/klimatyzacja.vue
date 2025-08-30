<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title card__title--lights">Klimatyzacja</h2>
        </div>
        <div class="card__body card__body--slider">
            <div class="card__content card__content--slider">
                <form class="temp-form">
  <input type="number" min="16" max="30" step="1" placeholder="Wprowadź temperaturę" class="temp-input" required />
  <button type="submit" class="btn">Ustaw temperaturę</button>
</form>

<label class="switch">
  <input type="checkbox">
  <span class="slider round"></span>
</label>
               
            </div>
        </div>
    </div>
</template>

<script setup>


</script>

<style lang="scss" scoped>

.temp-form {
  display: flex;
  gap: 1rem;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  margin-bottom: 3rem;
}

.temp-input {
  padding: 8px;
  font-size: 12pt;
  width: 14rem;
  border: 2px solid #ccc;
  border-radius: 4px;
  outline: none;
  transition: border-color 0.3s;
  display: block;
}

.temp-input:focus {
  border-color: var(--color);
}


$slider-switch-size: 3.5rem;

.card__body--slider {
  display: grid;
  justify-content: center;
  align-items: center;
}

.card__content--slider {
  height: fit-content;
}

/* The switch - the box around the slider */
.switch {
  position: relative;
  display: inline-block;
  width: 11rem;
  height: 4rem;
}

/* Hide default HTML checkbox */
.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

/* The slider */
.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #222;
  border-radius: 34px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 1.4rem;
  font-weight: bold;
  color: white;
  transition: background-color .4s, box-shadow .4s;
  overflow: hidden;
}

/* Domyślny tekst OFF */
.slider::after {
  content: "OFF";
  opacity: 1;
  transition: opacity .3s ease-in-out;
  position: relative;
  z-index: 2;
}

/* Wewnętrzne kółko */
.slider:before {
  position: absolute;
  content: "";
  height: $slider-switch-size;
  width: $slider-switch-size;
  left: 4px;
  bottom: 4px;
  background-color: white;
  border-radius: 50%;
  transition: .4s;
  z-index: 1;
}

/* Efekt glow przy ON */
input:checked + .slider {
  box-shadow: 0 0 25px 6px #d40fe6; /* glow */
}

/* Zmiana tekstu ON/OFF z fade */
input:checked + .slider::after {
  content: "ON";
  opacity: 1;
  animation: fadeText .3s ease-in-out;
}

/* Gdy zaznaczony przesuwamy kółko w prawo */
input:checked + .slider:before {
  transform: translateX(7rem);
}

/* Animacja fade dla tekstu */
@keyframes fadeText {
  0% { opacity: 0; }
  50% { opacity: 0.5; }
  100% { opacity: 1; }
}




</style>