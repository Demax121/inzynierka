<template>
  <nav class="navbar" aria-label="Primary Navigation">
    <div class="navbar__container">
      <div class="navbar__logo" role="banner">
        <h1 class="navbar__brand">Smart Home</h1>
      </div>
      <ul class="navbar__list" role="menubar">
        <li class="navbar__item" role="none">
          <button class="navbar__link" role="menuitem" @click="$emit('changeTab', 'HomeTab')">Home</button>
        </li>
        <li class="navbar__item" role="none">
          <button class="navbar__link" role="menuitem" @click="$emit('changeTab', 'WeatherTab')">Weather</button>
        </li>
        <li class="navbar__item" role="none">
          <button class="navbar__link" role="menuitem" @click="$emit('changeTab', 'WledTab')">WLED</button>
        </li>
        <li class="navbar__item" role="none">
          <button class="navbar__link" role="menuitem" @click="$emit('changeTab', 'ProfilesTab')">Profiles</button>
        </li>
      </ul>
    </div>
  </nav>
</template>

<script setup>

</script>

<style lang="scss" scoped>
@use "@/scss/colors" as *;

.navbar {
  position: sticky;
  top: 0;
  left: 0;
  right: 0;
  z-index: 1000;
  background-color: $title-crl-secondary;
  box-shadow: 0 0.125rem 0.25rem rgba(0, 0, 0, 0.1); /* 2px 4px */
  backdrop-filter: blur(0.625rem); /* 10px */
  height: var(--nav-height, 3.75rem);

  &__container {
    max-width: 75rem; /* 1200px */
    margin: 0 auto;
    padding: 0 1rem;
    display: flex;
    justify-content: space-between;
    align-items: center;
    height: 100%;
    gap: 2rem;
    box-sizing: border-box;
  }

  &__logo { margin: 0; }

  &__brand {
    color: $title-crl-primary;
    font-size: 1.5rem;
    font-weight: bold;
    margin: 0;
  }

  &__list {
    list-style: none;
    display: flex;
    margin: 0;
    padding: 0;
    gap: 2rem;
  }

  &__item { margin: 0; }

  &__link {
    background: none;
    border: none;
    color: #fff;
    text-decoration: none;
    font-weight: 500;
    padding: 0.5rem 1rem;
    border-radius: 0.25rem; /* 4px */
    transition: background-color .3s ease, color .3s ease, opacity .3s ease;
    cursor: pointer;
    font-size: inherit;
    font-family: inherit;
    position: relative;

    &:hover {
      background-color: $btn-hover-crl-primary;
      color: $title-crl-primary;
    }

    &--active {
      background-color: $title-crl-primary;
      color: #fff;
    }

    &:focus-visible {
      outline: 0.125rem solid $title-crl-primary; /* 2px */
      outline-offset: 0.125rem; /* 2px */
    }
  }
}
</style>
