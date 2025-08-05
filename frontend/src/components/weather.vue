<template>
    <div class="card card--weather">
        <div class="card__header">
            <h2 class="card__title">Pogoda</h2>
        </div>
        <div class="card__body">
            <div class="card__content">
                <iframe 
                    width="100%" 
                    height="450" 
                    src="https://embed.windy.com/embed.html?type=map&location=coordinates&metricRain=mm&metricTemp=°C&metricWind=km/h&zoom=11&overlay=wind&product=ecmwf&level=surface&lat=52.746&lon=15.21&detailLat=52.72714412683363&detailLon=15.23460388183594&marker=true" 
                    frameborder="0"
                    class="weather-iframe">
                </iframe>
            </div>
        </div>
    </div>
</template>

<script setup>

</script>

<style lang="scss" scoped>
.weather-iframe {
    border-radius: 8px;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
    transition: box-shadow 0.2s ease;
    
    &:hover {
        box-shadow: 0 4px 16px rgba(0, 0, 0, 0.15);
    }
}

// Make the weather component wider and more prominent
.card {
    margin-bottom: 2rem;
    
    &__content {
        padding: 0; // Remove padding to maximize iframe space
    }
    &--weather{
        max-height: fit-content;
    }
}

// Responsive design for different screen sizes
@media (max-width: 768px) {
    .weather-iframe {
        height: 350px; // Smaller height on mobile
    }
}

@media (min-width: 1200px) {
    .weather-iframe {
        height: 500px; // Even larger on desktop
    }
}

@media (min-width: 1600px) {
    .weather-iframe {
        height: 550px; // Maximum size for very large screens
    }
}
</style>