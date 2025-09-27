<template>
  <nav class="navbar" aria-label="Primary Navigation">
    <div class="navbar__container">
      <div class="navbar__logo" role="banner">
        <h1 class="navbar__brand">Simply Smart</h1>
      </div>
      <!-- Nazwa aktywnego taba na mobile -->
      <span class="navbar__active-tab" v-if="activeTabLabel" aria-current="page">{{ activeTabLabel }}</span>
      <!-- Hamburger for mobile -->
      <button class="navbar__hamburger" @click="toggleMenu" aria-label="Open menu" :aria-expanded="menuOpen">
        <span></span><span></span><span></span>
      </button>
      <ul
        class="navbar__list"
        :class="{ 'navbar__list--open': menuOpen }"
        role="menubar"
      >
        <li class="navbar__item" role="none">
          <button class="navbar__link" role="menuitem" @click="selectTab('HomeTab')">Home</button>
        </li>
        <li class="navbar__item" role="none">
          <button class="navbar__link" role="menuitem" @click="selectTab('WeatherTab')">Weather</button>
        </li>
        <li class="navbar__item" role="none">
          <button class="navbar__link" role="menuitem" @click="selectTab('WledTab')">WLED</button>
        </li>
        <li class="navbar__item" role="none">
          <button class="navbar__link" role="menuitem" @click="selectTab('ProfilesTab')">Profiles</button>
        </li>
      </ul>
    </div>
  </nav>
</template>

<script setup>
import { ref, computed, defineProps } from 'vue';
const menuOpen = ref(false);
const toggleMenu = () => { menuOpen.value = !menuOpen.value; };
const emit = defineEmits(['changeTab']);

// Przyjmij aktywny tab jako props
const props = defineProps({
  activeTab: {
    type: String,
    default: 'HomeTab'
  }
});

const tabLabels = {
  HomeTab: 'Home',
  WeatherTab: 'Weather',
  WledTab: 'Wled',        // <- zmień na 'Wled'
  ProfilesTab: 'Profiles'
};

const activeTabLabel = computed(() => tabLabels[props.activeTab] || '');

const selectTab = (tab) => {
  menuOpen.value = false;
  emit('changeTab', tab);
};
</script>

<style lang="scss" scoped>
@use "@/scss/colors" as *;

// Mixin for mobile
@mixin mobile {
  @media (max-width: 600px) {
    @content;
  }
}

.navbar {
  position: sticky;
  top: 0;
  left: 0;
  right: 0;
  z-index: 1000;
  background-color: $title-crl-secondary;
  box-shadow: 0 0.125rem 0.25rem rgba(0, 0, 0, 0.1);
  backdrop-filter: blur(0.625rem);
  height: var(--nav-height, 3.75rem);

  &__container {
    max-width: 75rem;
    margin: 0 auto;
    padding: 0 1rem;
    display: flex;
    justify-content: space-between;
    align-items: center;
    height: 100%;
    gap: 2rem;
    box-sizing: border-box;
    position: relative;
  }

  &__logo {
    margin: 0;
    display: flex;
    align-items: center;
  }

  &__brand {
    color: $title-crl-primary;
    font-size: 1.1rem; // mniejsze logo
    font-weight: bold;
    margin: 0;
    letter-spacing: 0.02em;
    @include mobile {
      font-size: 1rem;
      
    }
  }

  &__active-tab {
    display: none;
    @include mobile {
      display: inline-block;
      color: $text-crl-secondary;
      text-transform: uppercase;
      font-size: 1.25rem;
      font-weight: 500;
      margin-left: 1rem;
      margin-right: auto;
      white-space: nowrap;
      max-width: 8rem;
      overflow: hidden;
      text-overflow: ellipsis;
    }
  }

  &__hamburger {
    display: none;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    width: 2.25rem;
    height: 2.25rem;
    background: none;
    border: none;
    cursor: pointer;
    z-index: 1100;
    span {
      display: block;
      width: 1.7rem;
      height: 0.2rem;
      margin: 0.2rem 0;
      background: $title-crl-primary;
      border-radius: 2px;
      transition: all 0.3s;
    }
    @include mobile {
      display: flex;
    }
  }

  &__list {
    list-style: none;
    display: flex;
    margin: 0;
    padding: 0;
    gap: 2rem;
    background: none;
    position: static;
    transition: none;

    @include mobile {
      flex-direction: column;
      gap: 0;
      position: absolute;
      top: 100%;
      right: 0;
      left: 0;
      background: $title-crl-secondary;
      box-shadow: 0 0.25rem 1rem rgba(0,0,0,0.08);
      border-radius: 0 0 0.5rem 0.5rem;
      overflow: hidden;
      max-height: 0;
      opacity: 0;
      pointer-events: none;
      transition: max-height 0.3s, opacity 0.3s;
      z-index: 1050;
    }

    &--open {
      @include mobile {
        max-height: 20rem;
        opacity: 1;
        pointer-events: auto;
        transition: max-height 0.3s, opacity 0.3s;
      }
    }
  }

  &__item {
    margin: 0;
    @include mobile {
      width: 100%;
    }
  }

  &__link {
    background: none;
    border: none;
    color: #fff;
    text-decoration: none;
    font-weight: 500;
    padding: 0.5rem 1rem;
    border-radius: 0.25rem;
    transition: background-color .3s, color .3s, opacity .3s;
    cursor: pointer;
    font-size: inherit;
    font-family: inherit;
    position: relative;
    width: 100%;
    text-align: left;

    &:hover {
      background-color: $btn-hover-crl-primary;
      color: $title-crl-primary;
    }

    &--active {
      background-color: $title-crl-primary;
      color: #fff;
    }

    &:focus-visible {
      outline: 0.125rem solid $title-crl-primary;
      outline-offset: 0.125rem;
    }
    @include mobile {
      width: 100%;
      border-radius: 0;
      padding: 1rem;
      border-bottom: 1px solid rgba(255,255,255,0.08);
    }
  }
}
</style>
