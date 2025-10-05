<template>
  <!-- Primary site navigation. Accessible: uses nav landmark, menubar roles, aria-current on active label (mobile). -->
  <nav class="navbar" aria-label="Primary Navigation">
    <div class="navbar__container">
      <!-- Brand / Logo -->
      <div class="navbar__logo" role="banner">
        <h1 class="navbar__brand">Simply Smart</h1>
      </div>
      <!-- Active tab name (only visible on mobile view) -->
      <span class="navbar__active-tab" v-if="activeTabLabel" aria-current="page">{{ activeTabLabel }}</span>
      <!-- Hamburger toggle (mobile). aria-expanded bound to reactive open state -->
      <button
        class="navbar__hamburger"
        :class="{ 'navbar__hamburger--open': menuOpen }"
        @click="toggleMenu"
        :aria-label="menuOpen ? 'Close menu' : 'Open menu'"
        :aria-expanded="menuOpen"
      >
        <span></span><span></span><span></span>
      </button>
      <!-- Navigation list: collapses into sliding panel on mobile -->
      <ul class="navbar__list" :class="{ 'navbar__list--open': menuOpen }" role="menubar">
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
                <li class="navbar__item" role="none">
          <button class="navbar__link" role="menuitem" @click="selectTab('DoorStatusTab')">Door Status log</button>
        </li>
                <li class="navbar__item" role="none">
          <button class="navbar__link" role="menuitem" @click="selectTab('RoomStatsTab')">Room statistics log</button>
        </li>
      </ul>
    </div>
  </nav>
</template>

<script setup>
// Navbar component: handles responsive menu toggle and tab selection.
// Emits: changeTab(newTabKey)

import { ref, computed, defineProps } from 'vue';

// Reactive open/closed state for mobile menu
const menuOpen = ref(false);
const toggleMenu = () => { menuOpen.value = !menuOpen.value; };

// Emits definition
const emit = defineEmits(['changeTab']);

// Accept currently active tab (controlled by parent)
const props = defineProps({
  activeTab: { type: String, default: 'HomeTab' }
});

// Mapping from internal tab keys to user-facing labels
const tabLabels = {
  HomeTab: 'Home',
  WeatherTab: 'Weather',
  WledTab: 'Wled', // front label formatting
  ProfilesTab: 'Profiles'
};

// Derive label or empty fallback (prevents flashing undefined)
const activeTabLabel = computed(() => tabLabels[props.activeTab] || '');

// On selection: close menu (for mobile) then emit new tab key upward
const selectTab = (tab) => {
  menuOpen.value = false;
  emit('changeTab', tab);
};
</script>

<style lang="scss" scoped>
@use "@/scss/colors" as *;

// Design tokens (local to navbar component)
$nb-max-width: 75rem;
$nb-padding-x: 1rem;
$nb-gap: 2rem;
$nb-height: var(--nav-height, 3.75rem);
$nb-hamburger-size: 2.25rem;
$nb-hamburger-bar-width: 1.7rem;
$nb-hamburger-bar-height: 0.2rem;
$nb-mobile-breakpoint: 600px;
$nb-active-tab-max-width: 8rem;
$nb-transition: 0.3s;
$nb-radius: 0.5rem;
$nb-link-padding-y: 0.5rem;
$nb-link-padding-x: 1rem;
$nb-link-radius: 0.25rem;
$nb-mobile-link-padding: 1rem; 
$nb-hamburger-cross-offset: 0.4rem; // vertical shift when forming X


// Mobile mixin
@mixin mobile { @media (max-width: $nb-mobile-breakpoint) { @content; } }

.navbar {
  position: sticky; top: 0; left: 0; right: 0; z-index: 1000;
  background-color: $title-crl-secondary;
  box-shadow: 0 0.125rem 0.25rem rgba(0, 0, 0, 0.1);
  backdrop-filter: blur(0.625rem);
  height: $nb-height;

  &__container {
    max-width: $nb-max-width; margin:0 auto; padding:0 $nb-padding-x; display:flex;
    justify-content:space-between; align-items:center; height:100%; gap:$nb-gap; box-sizing:border-box; position:relative;
  }

  &__logo { margin:0; display:flex; align-items:center; }

  &__brand { color:$title-crl-primary; font-size:1.1rem; font-weight:bold; margin:0; letter-spacing:0.02em; @include mobile { font-size:1rem; } }

  &__active-tab { display:none; @include mobile { display:inline-block; color:$text-crl-secondary; text-transform:uppercase; font-size:1.25rem; font-weight:500; margin-left:1rem; margin-right:auto; white-space:nowrap; max-width:$nb-active-tab-max-width; overflow:hidden; text-overflow:ellipsis; } }

  &__hamburger {
    display:none; flex-direction:column; justify-content:center; align-items:center;
    width:$nb-hamburger-size; height:$nb-hamburger-size; background:none; border:none; cursor:pointer; z-index:1100;
    span { display:block; width:$nb-hamburger-bar-width; height:$nb-hamburger-bar-height; margin:0.2rem 0; background:$title-crl-primary; border-radius:2px; transition:all $nb-transition; transform-origin:center; }
    @include mobile { display:flex; }
  }

  /* Open (X) state transformations */
  &__hamburger--open {
    span:nth-child(1) { transform: translateY($nb-hamburger-cross-offset) rotate(45deg); }
    span:nth-child(2) { opacity:0; }
    span:nth-child(3) { transform: translateY(-$nb-hamburger-cross-offset) rotate(-45deg); }
  }

  &__list {
    list-style:none; display:flex; margin:0; padding:0; gap:$nb-gap; background:none; position:static; transition:none;
    @include mobile {
      flex-direction:column; gap:0; position:absolute; top:100%; right:0; left:0; background:$title-crl-secondary;
      box-shadow:0 0.25rem 1rem rgba(0,0,0,0.08); border-radius:0 0 $nb-radius $nb-radius; overflow:hidden;
      max-height:0; opacity:0; pointer-events:none; transition:max-height $nb-transition, opacity $nb-transition; z-index:1050;
    }
    &--open { @include mobile { max-height:20rem; opacity:1; pointer-events:auto; transition:max-height $nb-transition, opacity $nb-transition; } }
  }

  &__item { margin:0; @include mobile { width:100%; } }

  &__link {
    background:none; border:none; color:#fff; text-decoration:none; font-weight:500; padding:$nb-link-padding-y $nb-link-padding-x; border-radius:$nb-link-radius;
    transition:background-color $nb-transition, color $nb-transition, opacity $nb-transition; cursor:pointer; font-size:inherit; font-family:inherit; position:relative; width:100%; text-align:left;
    &:hover { background-color:$btn-hover-crl-primary; color:$title-crl-primary; }
    &--active { background-color:$title-crl-primary; color:#fff; }
    &:focus-visible { outline:0.125rem solid $title-crl-primary; outline-offset:0.125rem; }
    @include mobile { width:100%; border-radius:0; padding:$nb-mobile-link-padding; border-bottom:1px solid rgba(255,255,255,0.08); }
  }
}
</style>
