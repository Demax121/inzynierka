<template>
        <div class="hero">
            <div class="hero__text">
                <h1 class="hero__title">Welcome to Simply Smart UI</h1>
                <p class="hero__subtitle">Centralized hub for your smart devices.</p>
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
    if (!el){
    let el = document.querySelector('.big-container');
    el.scrollIntoView({ behavior: 'smooth' });
    };
    el.scrollIntoView({ behavior: 'smooth' });
};

</script>

<style lang="scss" scoped>
// BEM: .hero (block), __element, --modifier

// Mixin dla urządzeń mobilnych
@mixin mobile {
  @media (max-width: 600px) {
    @content;
  }
}

.hero {
    position: relative;
    width: 100%;
    height: 100dvh; // pełna wysokość ekranu, także na mobile
    min-height: 100dvh;
    max-height: 100dvh;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    text-align: center;
    overflow: hidden;

    &::before {
        content: '';
        position: absolute;
        inset: 0;
        background: linear-gradient(120deg, #fffbe6 0%, #ffe082 50%, #ffb74d 100%);
        opacity: 0.85;
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
        font-size: 72pt;
        font-weight: 600;
        font-family: inherit;
        @include mobile {
            font-size: 32pt;
        }
    }

    &__subtitle {
        position: relative;
        z-index: 1;
        margin: 0 0 1.25rem;
        font-size: 24pt;
        font-weight: 300;
        font-family: inherit;
        @include mobile {
            font-size: 14pt;
        }
    }

    &__scroll-btn {
        position: absolute;
        left: 50%;
        bottom: 1.5rem;
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
        @include mobile {
            bottom: 1rem;
        }
    }

    &__icon {
        max-width: 4rem;
        max-height: 4rem;
        margin: 0 auto;
        display: block;
        @include mobile {
            max-width: 2.5rem;
            max-height: 2.5rem;
        }
    }
}
</style>