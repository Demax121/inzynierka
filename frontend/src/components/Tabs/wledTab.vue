<template>
    <div class="big-container wled">
        <iframe
            class="wled__iframe"
            src="http://192.168.1.19/#Colors" frameborder="0"
            title="WLED Controller"
            loading="lazy"
        ></iframe>
    </div>
</template>

<script setup>

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