<script setup>

</script>

<template>
    <div class="big-container weather">
        <iframe
            class="weather__iframe"
            src="https://embed.windy.com/embed.html?type=map&location=coordinates&metricRain=mm&metricTemp=°C&metricWind=km/h&zoom=11&overlay=wind&product=ecmwf&level=surface&lat=52.746&lon=15.21&detailLat=52.72714412683363&detailLon=15.23460388183594&marker=true"
            title="Weather map"
            loading="lazy"
            referrerpolicy="no-referrer"
        ></iframe>
    </div>
</template>

<style lang="scss" scoped>
.weather {
  &__iframe {
    width: 100%;
    height: 100%;
    border: 0;
    display: block; /* removes inline gap */
  }
}
</style>