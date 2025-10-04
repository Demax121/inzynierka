<template>
    <!-- Card wrapper uses global .card styles; this component previously un-BEMed, left intact -->
    <div class="card">
        <div class="card__header">
            <h2 class="card__title">Blinds Control</h2>
        </div>
        <div class="card__body">
            <div class="card__content">

                <!-- Action buttons: manual control triggers Tuya via backend; automation is disabled first -->
                <div class="button-group">
                    <button type="button" class="btn" @click="openBlinds">
                        Open blinds
                    </button>

                    <button type="button" class="btn" @click="closeBlinds">
                        Close blinds
                    </button>
                    <button type="button" class="btn" @click="fetchStatus()">
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
// deviceState: canonical string 'open' | 'close' | null (derived from various Tuya keys)
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

// Generic helper contacting tuyaBlindsApi.php with optional action (open|close|status)
// On success for movement commands schedules follow-up status fetch (after delay in open/close wrappers).
const makeApiCall = async (actionParam) => {
    try {
        let apiUrl = linkStore.getPhpApiUrl('tuyaBlindsApi.php')
        if (actionParam) {
            apiUrl += `?action=${actionParam}`
        }

        const response = await fetch(apiUrl, {
            method: 'GET',
            headers: {
                'Content-Type': 'application/json',
            },
        })

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`)
        }

        const data = await response.json()
        responseData.value = data

        if (data.success) {
            fetchStatus().catch(() => {})
        }
    } catch (error) {
        console.error('API error:', error)
    }
}

// Manual open wrapper: disable automation, send command, schedule status refresh.
const openBlinds = async () => {
    await setAutomateFalse()
    await makeApiCall('open')
    setTimeout(fetchStatus, 5000);
}

// Manual close wrapper (mirrors open sequence).
const closeBlinds = async () => {
    await setAutomateFalse()
    await makeApiCall('close')
    setTimeout(fetchStatus, 5000);
}

// Parse legacy / verbose Tuya status shape: collect dp entries (list/status arrays) and infer battery & state.
// This is only used when simplified fields are absent.
const parseStatus = (data) => {
    
    batteryLevel.value = null
    deviceState.value = null

    if (!data) return

    const result = data.result ?? data

    let items = []
    if (Array.isArray(result)) {
        items = result
    } else if (result && typeof result === 'object') {

        items = result.list ?? result.status ?? []
        if (!Array.isArray(items)) items = []
    }

    const numericStateCodes = ['control', 'state', 'switch', 'status']
    const textStateCodes = ['control', 'state', 'switch', 'status']

    for (const it of items) {
        const code = (it.code || '').toLowerCase()
        const value = it.value

        if (code.includes('battery') && (typeof value === 'number' || typeof value === 'string')) {
            const num = Number(value)
            if (!Number.isNaN(num)) batteryLevel.value = num
        }

        if (typeof value === 'string' && (textStateCodes.includes(code) || code.includes('control'))) {
            const v = value.toLowerCase()
            if (v.includes('open')) deviceState.value = 'open'
            if (v.includes('close')) deviceState.value = 'close'
            continue
        }

        if (typeof value === 'number' && numericStateCodes.includes(code)) {
            if (value === 1) deviceState.value = 'open'
            else if (value === 0) deviceState.value = 'close'
        } else if (typeof value === 'boolean' && (textStateCodes.includes(code) || code.includes('control'))) {
            deviceState.value = value ? 'open' : 'close'
        }
    }
}

// Fetch current blinds info (prefers simplified structure with battery_percent & blinds_state).
// Falls back to parseStatus if those keys are absent, maintaining compatibility.
const fetchStatus = async () => {
    try {
        const apiUrl = linkStore.getPhpApiUrl('tuyaBlindsApi.php?action=status')
        const resp = await fetch(apiUrl, { method: 'GET' })
        if (!resp.ok) throw new Error(`HTTP ${resp.status}`)
        const data = await resp.json()
        responseData.value = data
        
        // Handle the new response format directly
        if (data) {
            if (data.battery_percent !== undefined && data.battery_percent !== null) {
                batteryLevel.value = Number(data.battery_percent)
            }
            if (data.blinds_state !== undefined) {
                const state = String(data.blinds_state).toLowerCase()
                if (state === 'open' || state === '1' || state === 'true') {
                    deviceState.value = 'open'
                } else if (state === 'close' || state === '0' || state === 'false') {
                    deviceState.value = 'close'
                }
            } else {
                parseStatus(data)
            }
        }
    } catch (err) {
        // fetchStatus error handled silently
    }
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
// Polling interval (ms) - adjust as needed
const POLL_INTERVAL_MS = 60_000 // 1 minute
let pollHandle = null

onMounted(() => {
    fetchStatus().catch(() => { })
    pollHandle = setInterval(() => {
        fetchStatus().catch(() => {})
    }, POLL_INTERVAL_MS)
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