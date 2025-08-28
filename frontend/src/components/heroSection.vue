<template>
        <div class="hero">
            <div class="hero__text">
                <h1 class="hero__title">Welcome to Smart Home</h1>
                <p class="hero__subtitle">Your centralized hub for managing all smart devices.</p>
            </div>
            <button class="hero__scroll-btn" type="button" @click="scrollToMainContainer">
                <img class="hero__icon" :src="linkStore.getImage('arrow-down-icon.svg')" alt="Scroll down" />
            </button>
        </div>
</template>

<script setup>

import { useLinkStore } from '@/stores/linkStore';
const linkStore = useLinkStore();
const scrollToMainContainer = () => {
    const el = document.querySelector('.main-container');
    if (!el) return;
    el.scrollIntoView({ behavior: 'smooth' });
};



</script>

<style lang="scss" scoped>
// BEM: .hero (block), __element, --modifier
.hero {
    position: relative;
    width: 100%;
    height: 100vh;
    display: flex;
    flex-direction: column;
    justify-content: center; // centers text container
    align-items: center;
    text-align: center;
    overflow: hidden;

    &::before { // decorative background (not an element)
        content: '';
        position: absolute;
        inset: 0;
        background: url('@/static assets/bg-photo-1.jpg') center/cover no-repeat;
        filter: blur(4px);
        transform: scale(1.05);
        z-index: 0;
    }

    &__text {
        position: relative;
        z-index: 1;
        margin: 0;
        display: flex;
        flex-direction: column;
        justify-content: center;
        max-width: 70ch;
    }

    &__title {
        position: relative;
        z-index: 1;
        margin: 0 0 .75rem;
        font-size: 32pt;
        font-weight: 600;
    }

    &__subtitle {
        position: relative;
        z-index: 1;
        margin: 0 0 1.25rem;
        font-size: 24pt;
        font-weight: 300;
    }

    &__scroll-btn {
        position: absolute;
        left: 50%;
        bottom: .25rem;
        transform: translateX(-50%);
        z-index: 1;
        background: transparent;
        border: none;
        cursor: pointer;
        width: 100%;
        border-top: 1px solid #fff;
        padding: .75rem 0 0;
        transition: opacity .3s ease;
        &:hover { opacity: .75; }
        &:focus-visible { outline: 2px solid #fff; outline-offset: 4px; }
    }

    &__icon {
        max-width: 5rem;
        max-height: 5rem;
        margin: 0 auto;
        display: block;
    }
}
</style>