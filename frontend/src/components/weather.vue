<template>
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Pogoda</h2>
        </div>
        <div class="card__body">
            <div class="card__content">
                <iframe 
                    width="100%" 
                    height="280" 
                    src="https://embed.windy.com/embed.html?type=map&location=coordinates&metricRain=mm&metricTemp=°C&metricWind=km/h&zoom=11&overlay=wind&product=ecmwf&level=surface&lat=52.746&lon=15.21&detailLat=52.72714412683363&detailLon=15.23460388183594&marker=true" 
                    frameborder="0"
                    style="border-radius: 8px;">
                </iframe>
            </div>
        </div>
    </div>
</template>

<script setup>

</script>

<style lang="scss" scoped>

</style>