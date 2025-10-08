<template>
    <!-- Card wrapper uses global .card styles; this component previously un-BEMed, left intact -->
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Blinds Control</h2>
        </div>
        <div class="card__body">
            <div class="card__content">

                <!-- Action buttons now call blindsControl.php backend; automation is disabled first -->
                <div class="button-group">
                    <button type="button" class="btn" @click="openBlinds">
                        Open blinds
                    </button>

                    <button type="button" class="btn" @click="closeBlinds">
                        Close blinds
                    </button>
                    <button type="button" class="btn" @click="manualFetchStatus()">
                        Get status
                    </button>
                </div>
                <!-- Device info: rendered only when at least one of battery or state is known -->
                <div class="device-info" v-if="batteryLevel !== null || deviceState !== null">
                    <!-- Battery percentage (raw value from either simplified API or parsed list). -->
                    <p class="card__text battery-line">
                        <b>Battery:</b>
                        <span class="battery-percentage">{{ batteryLevel !== null ? batteryLevel + '%' : 'No data' }}</span>
                        <!-- Inline SVG so we can use currentColor for dynamic tint -->
                        <svg
                            v-if="batteryLevel !== null"
                            class="battery-icon"
                            :class="batteryLevelClass"
                            :title="batteryTooltip"
                            viewBox="0 0 24 24"
                            role="img"
                            :aria-label="`Battery level ${batteryLevel}%`"
                            xmlns="http://www.w3.org/2000/svg"
                        >
                            <!-- Outline + terminal (merged from original asset); fill uses currentColor -->
                            <path fill="currentColor" fill-rule="evenodd" clip-rule="evenodd" d="M21 10c.552 0 1 .448 1 1v2c0 .552-.448 1-1 1s-1-.448-1-1v-2c0-.552.448-1 1-1Zm-14.066-3H14.066c.886 0 1.65 0 2.262.082.656.088 1.284.287 1.793.797.51.51.709 1.138.797 1.793.082.612.082 1.376.082 2.262v.132c0 .886 0 1.65-.082 2.262-.088.656-.287 1.284-.797 1.793-.51.51-1.138.709-1.793.797-.612.082-1.376.082-2.262.082H6.934c-.886 0-1.65 0-2.262-.082-.656-.088-1.284-.287-1.793-.797-.51-.51-.709-1.138-.797-1.793C2 13.65 2 12.886 2 12v-.132c0-.886 0-1.65.082-2.262.088-.656.287-1.284.797-1.793.51-.51 1.138-.709 1.793-.797C5.284 7 6.048 7 6.934 7Z" />
                        </svg>
                    </p>
                    <!-- Normalized state label produced by computed property -->
                    <p class="card__text"><b>Blinds state: </b> {{ deviceStateText }}</p>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
// == Imports ==
import { ref, onMounted, onBeforeUnmount, computed } from 'vue'
import { useLinkStore } from '@/stores/linkStore'
import { useAutomateStore } from '@/stores/automateStore'
// (Removed <img> usage; SVG now inlined for currentColor support.)


// == Reactive state holders ==
// responseData: raw last response (for optional debugging / future inspection)
const responseData = ref(null)
// batteryLevel: numeric percentage (null until known)
const batteryLevel = ref(null)
// deviceState: canonical string 'open' | 'close' | null
const deviceState = ref(null) 

// Pinia stores provide API URL construction and global automation flag
const linkStore = useLinkStore()
const automateStore = useAutomateStore()

// Disable blinds automation before manual movement so automation logic does not immediately override the command.
const setAutomateFalse = async () => {
    automateStore.automate_flag = false
    try {
        await fetch(linkStore.getPhpApiUrl('setBlindsAutomate.php'), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ automate: false })
        })
    } catch (e) {
        // Silent error
    }
}

// Generic helper contacting blindsControl.php with action=open|close|status
const makeApiCall = async (actionParam) => {
    if (!actionParam) return
    try {
        const apiUrl = linkStore.getPhpApiUrl('blindsControl.php') + `?action=${actionParam}`
        const response = await fetch(apiUrl, { method: 'GET' })
        if (!response.ok) throw new Error(`HTTP ${response.status}`)
        const data = await response.json()
        responseData.value = data
        // Map new JSON { state, battery }
        if (typeof data.battery !== 'undefined') {
            const num = Number(data.battery)
            batteryLevel.value = Number.isNaN(num) ? null : num
        }
        if (typeof data.state === 'string') {
            const st = data.state.toLowerCase()
            if (st.includes('open')) deviceState.value = 'open'
            else if (st.includes('close')) deviceState.value = 'close'
            else deviceState.value = null
        }
    } catch (error) {
        // Silent failure (could extend for UI feedback later)
        console.error('API error:', error)
    }
}

// Manual open wrapper: disable automation, send command, then refresh status and reset polling.
const openBlinds = async () => {
    await setAutomateFalse()
    await makeApiCall('open')
    await fetchStatus()
    resetPolling()
}

// Manual close wrapper.
const closeBlinds = async () => {
    await setAutomateFalse()
    await makeApiCall('close')
    await fetchStatus()
    resetPolling()
}

// Legacy parser removed: blindsControl.php returns a simple JSON {state,battery}

// Fetch current blinds info via status action.
const fetchStatus = async () => {
    try {
        const apiUrl = linkStore.getPhpApiUrl('blindsControl.php') + '?action=status'
        const resp = await fetch(apiUrl, { method: 'GET' })
        if (!resp.ok) throw new Error(`HTTP ${resp.status}`)
        const data = await resp.json()
        responseData.value = data
        if (typeof data.battery !== 'undefined') {
            const num = Number(data.battery)
            batteryLevel.value = Number.isNaN(num) ? null : num
        }
        if (typeof data.state === 'string') {
            const st = data.state.toLowerCase()
            if (st.includes('open')) deviceState.value = 'open'
            else if (st.includes('close')) deviceState.value = 'close'
            else deviceState.value = null
        }
    } catch (err) {
        // silent
    }
}

// Manual status fetch resets polling cycle
const manualFetchStatus = async () => {
    await fetchStatus()
    resetPolling()
}

// Human friendly mapping for UI label.
const deviceStateText = computed(() => {
    if (deviceState.value === 'open') return 'Open'
    if (deviceState.value === 'close') return 'Closed'
    return 'Unknown'
})

// Compute battery level class thresholds:
// low: 0-30, medium: 31-60, high: 61-100
const batteryLevelClass = computed(() => {
    const lvl = batteryLevel.value
    if (lvl === null || Number.isNaN(lvl)) return 'battery-unknown'
    if (lvl <= 30) return 'battery-low'
    if (lvl <= 60) return 'battery-medium'
    return 'battery-high'
})

// Tooltip explaining classification ranges and current bracket
const batteryTooltip = computed(() => {
    const lvl = batteryLevel.value
    const ranges = 'Low: 0-30%, Medium: 31-60%, High: 61-100%'
    if (lvl === null || Number.isNaN(lvl)) return `Battery: unknown. ${ranges}`
    let status = 'High'
    if (lvl <= 30) status = 'Low'
    else if (lvl <= 60) status = 'Medium'
    return `Battery ${lvl}% (${status}). ${ranges}`
})

// Initial fetch on mount; silent catch keeps UI clean if first request fails.
// Polling every 10s with reset on manual actions
const POLL_INTERVAL_MS = 10_000
let pollHandle = null

const startPolling = () => {
    pollHandle = setInterval(() => {
        fetchStatus().catch(() => {})
    }, POLL_INTERVAL_MS)
}

const resetPolling = () => {
    if (pollHandle) clearInterval(pollHandle)
    startPolling()
}

onMounted(() => {
    fetchStatus().catch(() => {})
    startPolling()
})

onBeforeUnmount(() => {
    if (pollHandle) clearInterval(pollHandle)
})
</script>

<style lang="scss" scoped>
// Basic local styling tokens for button group & info panel.
$button-padding: 11px 15px;
$button-font-size: 12pt;

// Battery customization variables
$battery-icon-size: 2.2rem; // Overall rendered square size
$battery-color-low: #ff4d4f;
$battery-color-medium: #fadb14;
$battery-color-high: #52c41a;
$battery-color-unknown: #888;
$battery-gap: 0.1rem;

.button-group {
    margin-top: 0.5rem;
}

// Device info container: simple shadowed panel showing derived status.
.device-info {
    filter: drop-shadow(0 5px 10px 0 #ffffff);
    width: 15rem;
    height: 5rem;
    padding: 1rem;
    color: white;
    font-size: 13pt;
}

// Ensure inherited size respects card context.
.card__text {
    font-size: inherit;
}

/* Battery line for proper inline alignment */
.battery-line {
    display: flex;
    align-items: center;
    gap: $battery-gap;
}

.battery-percentage { min-width: 3.2rem; }

.battery-icon {
    width: $battery-icon-size;
    height: $battery-icon-size;
    flex-shrink: 0;
}

/* Threshold colors (SVG uses currentColor) */
.battery-low { color: $battery-color-low; }
.battery-medium { color: $battery-color-medium; }
.battery-high { color: $battery-color-high; }
.battery-unknown { color: $battery-color-unknown; }
</style>