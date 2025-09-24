<template>
  <div class="card__content card__content--loading">
    <div class="loading-icon">☁️</div>
    <p class="card__text">Oczekiwanie danych...</p>
  </div>
</template>

<style lang="scss" scoped>
.card__content--loading { display: flex; flex-direction: column; align-items: center; gap: .75rem; }
.loading-icon { font-size: 2.5rem; animation: float 2s ease-in-out infinite; }
@keyframes float { 0%,100%{ transform: translateY(0);} 50%{ transform: translateY(-6px);} }
</style>