<template>
    <div class="big-container wled">
        <iframe
            class="wled__iframe"
            :src="wledUrl"
            frameborder="0"
            title="WLED Controller"
            loading="lazy"
        ></iframe>
    </div>
</template>

<script setup>
import { useLinkStore } from '@/stores/linkStore';
import { computed } from 'vue';

const linkStore = useLinkStore();
const wledUrl = computed(() => `${linkStore.links.wledIP}/#Colors`);
</script>

<style lang="scss" scoped>
.wled {
  &__iframe {
    width: 100%;
    height: 100%;
    border: 0;
    display: block;
  }
}
</style>