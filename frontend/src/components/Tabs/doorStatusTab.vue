<template>
    <div class="big-container">
        <div class="tab__header">
            <h3>Door status history</h3>
            <div class="tab__actions">
                <button class="btn" @click="fetchData" :disabled="loading">Refresh</button>
            </div>
        </div>

        <div class="tab__body">
            <div v-if="loading" class="tab__loading">Loading…</div>
            <div v-else-if="error" class="tab__error">Error: {{ error }}</div>
            <table v-else class="door-table">
                <thead>
                    <tr>
                        <th>Date</th>
                        <th>Status</th>
                    </tr>
                </thead>
                <tbody>
                    <tr v-for="row in rows" :key="row.status_date + row.status_name">
                        <td>{{ formatDate(row.status_date) }}</td>
                        <td>{{ row.status_name }}</td>
                    </tr>
                </tbody>
            </table>
            <div v-if="rows.length === 0 && !loading" class="tab__empty">No records found.</div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted } from 'vue';
import { useLinkStore } from '@/stores/linkStore';

const linkStore = useLinkStore();

const rows = ref([]);
const loading = ref(false);
const error = ref(null);

const formatDate = (d) => {
    try {
        const dt = new Date(d);
        return dt.toLocaleString();
    } catch {
        return d;
    }
}

// Fetch from PHP endpoint ONLY (no websockets)
const fetchData = async () => {
    loading.value = true;
    error.value = null;
    try {
        const endpoint = linkStore.getPhpApiUrl('getDoorStatus.php');
        const res = await fetch(endpoint, { method: 'GET' });
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        const payload = await res.json();
        if (!payload || payload.success !== true) throw new Error(payload && payload.error ? payload.error : 'Invalid response');
        rows.value = Array.isArray(payload.data) ? payload.data : [];
    } catch (e) {
        error.value = e.message || String(e);
        rows.value = [];
    } finally {
        loading.value = false;
    }
}

onMounted(() => {
    fetchData();
});
</script>

<style lang="scss" scoped>
.door-status-tab { padding: 1rem; }
.tab__header { display:flex; justify-content:space-between; align-items:center; margin-bottom:0.5rem }
.tab__actions .btn { padding: 0.4rem 0.8rem }
.tab__loading, .tab__error, .tab__empty { padding: 0.5rem 0; color: #666 }
.door-table { width:100%; border-collapse: collapse }
.door-table th, .door-table td { text-align:left; padding: 0.5rem; border-bottom: 1px solid #eee }
</style>