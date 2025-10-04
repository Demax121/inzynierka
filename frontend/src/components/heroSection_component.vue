<template>
    <!-- Hero section: full-viewport introduction banner -->
    <div class="hero">
        <div class="hero__text">
            <h1 class="hero__title">Welcome to Simply Smart UI</h1>
            <p class="hero__subtitle">Centralized hub for your smart devices.</p>
        </div>
        <!-- Scroll button: smoothly scrolls to main content container -->
        <button class="hero__scroll-btn" type="button" @click="scrollToMainContainer" aria-label="Scroll to dashboard">
            <img class="hero__icon" :src="arrowDownIcon" alt="Scroll down" />
        </button>
    </div>
</template>

<script setup>
// Asset import (arrow icon SVG)
import arrowDownIcon from '@/static assets/arrow-down-icon.svg';

// Scroll handler:
// Attempts to find primary grid container (.main-container),
// falls back to fullscreen variant (.big-container). No error if neither exists.
const scrollToMainContainer = () => {
    const target = document.querySelector('.main-container') || document.querySelector('.big-container');
    if (target) target.scrollIntoView({ behavior: 'smooth' });
};

</script>

<style lang="scss" scoped>
// BEM: .hero (block), __element; no modifiers currently.
// Extracting design tokens for easier maintenance.

// Design Tokens / Variables
$hero-vh: 100dvh; // full dynamic viewport height (handles mobile chrome)
$hero-gradient: linear-gradient(120deg, #fffbe6 0%, #ffe082 50%, #ffb74d 100%);
$hero-overlay-opacity: 0.85;
$hero-title-size: 72pt;
$hero-title-size-mobile: 32pt;
$hero-subtitle-size: 24pt;
$hero-subtitle-size-mobile: 14pt;
$hero-title-weight: 600;
$hero-subtitle-weight: 300;
$hero-icon-size: 4rem;
$hero-icon-size-mobile: 2.5rem;
$hero-scroll-bottom: 1.5rem;
$hero-scroll-bottom-mobile: 1rem;
$hero-scroll-border: 1px solid #fff;
$hero-focus-outline: 2px solid #fff;
$hero-transition: .3s ease;

// Mobile breakpoint mixin
@mixin mobile { @media (max-width: 600px) { @content; } }

.hero {
    position: relative;
    width: 100%;
    height: $hero-vh;
    min-height: $hero-vh;
    max-height: $hero-vh;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    text-align: center;
    overflow: hidden;

    &::before { // Decorative gradient overlay
        content: '';
        position: absolute;
        inset: 0;
        background: $hero-gradient;
        opacity: $hero-overlay-opacity;
        z-index: 0;
    }

    &__text {
        position: relative;
        z-index: 1;
        margin: 0;
        display: flex;
        flex-direction: column;
        justify-content: center;
        max-width: fit-content;
    }

    &__title {
        position: relative;
        z-index: 1;
        margin: 0 0 .75rem;
        font-size: $hero-title-size;
        font-weight: $hero-title-weight;
        font-family: inherit;
        @include mobile { font-size: $hero-title-size-mobile; }
    }

    &__subtitle {
        position: relative;
        z-index: 1;
        margin: 0 0 1.25rem;
        font-size: $hero-subtitle-size;
        font-weight: $hero-subtitle-weight;
        font-family: inherit;
        @include mobile { font-size: $hero-subtitle-size-mobile; }
    }

    &__scroll-btn {
        position: absolute;
        left: 50%;
        bottom: $hero-scroll-bottom;
        transform: translateX(-50%);
        z-index: 1;
        background: transparent;
        border: none;
        cursor: pointer;
        width: 100%;
        border-top: $hero-scroll-border;
        padding: .75rem 0 0;
        transition: opacity $hero-transition;
        &:hover { opacity: .75; }
        &:focus-visible { outline: $hero-focus-outline; outline-offset: 4px; }
        @include mobile { bottom: $hero-scroll-bottom-mobile; }
    }

    &__icon {
        max-width: $hero-icon-size;
        max-height: $hero-icon-size;
        margin: 0 auto;
        display: block;
        @include mobile {
            max-width: $hero-icon-size-mobile;
            max-height: $hero-icon-size-mobile;
        }
    }
}
</style>