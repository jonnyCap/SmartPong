<template>
  <div class="dashboard-wrapper glass-panel">
    <h3>Profile Stats ({{ viewModeLabel }})</h3>
    
    <div v-if="userProfile && filteredThrows.length > 0" class="stats-grid">
      <div class="stat-card">
        <span class="label">Selected Throws</span>
        <span class="value">{{ filteredThrows.length }}</span>
      </div>
      <div class="stat-card">
        <span class="label">Hit Rate</span>
        <span class="value">{{ hitRate.toFixed(1) }}%</span>
      </div>
      <div class="stat-card">
        <span class="label">Accel (Selected)</span>
        <span class="value">{{ avgAccel.toFixed(2) }} g</span>
        <span class="subtext">Min: {{ minAccel.toFixed(2) }} | Max: {{ maxAccel.toFixed(2) }}</span>
      </div>
      <div class="stat-card">
        <span class="label">Drift (Selected)</span>
        <span class="value">{{ avgDrift.toFixed(1) }}°</span>
        <span class="subtext">Min: {{ minDrift.toFixed(1) }}° | Max: {{ maxDrift.toFixed(1) }}°</span>
      </div>
    </div>
    <div v-else-if="userProfile" class="loading">No throws found for this selection.</div>
    <div v-else class="loading">Loading profile data...</div>

    <h3 class="mt-4">Control Quality</h3>
    <div v-if="userProfile && filteredThrows.length > 0" class="tile-grid">
      <div v-for="(pct, cat) in classPercentages.control" :key="cat" class="class-tile" :class="getColorClass(cat)">
        <span class="tile-name">{{ cat }}</span>
        <span class="tile-pct">{{ pct }}</span>
      </div>
    </div>

    <h3 class="mt-4">Drift Quality</h3>
    <div v-if="userProfile && filteredThrows.length > 0" class="tile-grid">
      <div v-for="(pct, cat) in classPercentages.drift" :key="cat" class="class-tile" :class="getColorClass(cat)">
        <span class="tile-name">{{ cat }}</span>
        <span class="tile-pct">{{ pct }}</span>
      </div>
    </div>
    <div v-else class="loading">Waiting for data...</div>
    
  </div>
</template>

<script setup>
import { defineProps, computed } from 'vue';

const props = defineProps({
  userProfile: {
    type: Object,
    default: null
  },
  latestThrow: {
    type: Object,
    default: null
  },
  viewMode: {
    type: String,
    default: 'all'
  }
});

const viewModeLabel = computed(() => {
  if (props.viewMode === 'all') return 'All Throws';
  if (props.viewMode === 'hits') return 'Hits Only';
  if (props.viewMode === 'misses') return 'Misses Only';
  if (props.viewMode === 'latest') return 'Latest Throw';
  if (props.viewMode.startsWith('history-')) {
    const idx = parseInt(props.viewMode.split('-')[1]);
    const total = props.userProfile?.throws?.length || 0;
    return `Throw #${total - idx}`;
  }
  return 'Selected';
});

const filteredThrows = computed(() => {
  if (!props.userProfile || !props.userProfile.throws) return [];
  if (props.viewMode === 'all') return props.userProfile.throws;
  if (props.viewMode === 'hits') return props.userProfile.throws.filter(t => t.is_hit);
  if (props.viewMode === 'misses') return props.userProfile.throws.filter(t => !t.is_hit);
  if (props.viewMode === 'latest') {
    if (props.latestThrow) return [props.latestThrow]; // This has different structure (no path array, but has peak_accel, etc)
    const len = props.userProfile.throws.length;
    return len > 0 ? [props.userProfile.throws[len - 1]] : [];
  }
  if (props.viewMode.startsWith('history-')) {
    const idx = parseInt(props.viewMode.split('-')[1]);
    return props.userProfile.throws[idx] ? [props.userProfile.throws[idx]] : [];
  }
  return [];
});

const hitRate = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  const hits = filteredThrows.value.filter(t => t.is_hit || t.hit).length; // Handle both throw object formats
  return (hits / filteredThrows.value.length) * 100;
});

const avgAccel = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return filteredThrows.value.reduce((sum, t) => sum + t.peak_acceleration, 0) / filteredThrows.value.length;
});
const minAccel = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return Math.min(...filteredThrows.value.map(t => t.peak_acceleration));
});
const maxAccel = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return Math.max(...filteredThrows.value.map(t => t.peak_acceleration));
});

const avgDrift = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return filteredThrows.value.reduce((sum, t) => sum + (t.drift_angle || 0), 0) / filteredThrows.value.length;
});
const minDrift = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return Math.min(...filteredThrows.value.map(t => t.drift_angle || 0));
});
const maxDrift = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return Math.max(...filteredThrows.value.map(t => t.drift_angle || 0));
});

const classPercentages = computed(() => {
  const throws = filteredThrows.value;
  const count = throws.length;
  
  // Default values to ensure all categories show up even if 0%
  const control = { 'Controlled': 0, 'Moderately Controlled': 0, 'Uncontrolled': 0 };
  const drift = { 'Low Drift': 0, 'Moderate Drift': 0, 'Strong Drift': 0 };
  
  if (count === 0) {
    Object.keys(control).forEach(k => control[k] = '0.0%');
    Object.keys(drift).forEach(k => drift[k] = '0.0%');
    return { control, drift };
  }
  
  throws.forEach(t => {
     if (t.control_category) {
       control[t.control_category] = (control[t.control_category] || 0) + 1;
     }
     if (t.drift_category) {
       drift[t.drift_category] = (drift[t.drift_category] || 0) + 1;
     }
  });
  
  Object.keys(control).forEach(k => control[k] = ((control[k] / count) * 100).toFixed(1) + '%');
  Object.keys(drift).forEach(k => drift[k] = ((drift[k] / count) * 100).toFixed(1) + '%');
  
  return { control, drift };
});

const getColorClass = (cat) => {
  if (cat === 'Controlled' || cat === 'Low Drift') return 'tile-success';
  if (cat === 'Moderately Controlled' || cat === 'Moderate Drift') return 'tile-warning';
  if (cat === 'Uncontrolled' || cat === 'Strong Drift') return 'tile-danger';
  return '';
};

</script>

<style scoped>
.dashboard-wrapper {
  padding: 1.5rem;
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
}

h3 {
  color: var(--text-muted);
  font-size: 1rem;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  border-bottom: 1px solid var(--border-color);
  padding-bottom: 0.5rem;
  margin-bottom: 0.5rem;
}

.mt-4 {
  margin-top: 1rem;
}

.stats-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1rem;
}

.stat-card {
  background: rgba(0, 0, 0, 0.2);
  padding: 1rem;
  border-radius: 12px;
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
  border: 1px solid transparent;
  transition: all 0.3s ease;
}

.stat-card:hover {
  border-color: rgba(59, 130, 246, 0.3);
  transform: translateY(-2px);
  background: rgba(59, 130, 246, 0.05);
}

.label {
  font-size: 0.85rem;
  color: var(--text-muted);
}

.value {
  font-size: 1.5rem;
  font-weight: 700;
  color: var(--text-main);
}

.subtext {
  font-size: 0.85rem;
  color: var(--text-muted);
}

.tile-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 1rem;
}

.class-tile {
  background: rgba(0, 0, 0, 0.2);
  padding: 1.5rem 0.5rem;
  border-radius: 12px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  border: 1px solid transparent;
  transition: all 0.3s ease;
  text-align: center;
  gap: 0.5rem;
}

.class-tile:hover {
  border-color: rgba(59, 130, 246, 0.3);
  transform: translateY(-2px);
  background: rgba(59, 130, 246, 0.05);
}

.tile-name {
  font-size: 0.85rem;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.05em;
  min-height: 2.5em;
  display: flex;
  align-items: center;
  justify-content: center;
}

.tile-pct {
  font-size: 1.8rem;
  font-weight: 700;
  color: var(--text-main);
}

.loading {
  color: var(--text-muted);
  font-style: italic;
  font-size: 0.9rem;
  padding: 1rem 0;
}

.tile-success {
  background: rgba(16, 185, 129, 0.15);
  border-color: rgba(16, 185, 129, 0.3);
}
.tile-success:hover {
  background: rgba(16, 185, 129, 0.25);
  border-color: rgba(16, 185, 129, 0.5);
}
.tile-success .tile-name {
  color: #a7f3d0;
}
.tile-success .tile-pct {
  color: #10b981;
}

.tile-warning {
  background: rgba(245, 158, 11, 0.15);
  border-color: rgba(245, 158, 11, 0.3);
}
.tile-warning:hover {
  background: rgba(245, 158, 11, 0.25);
  border-color: rgba(245, 158, 11, 0.5);
}
.tile-warning .tile-name {
  color: #fde68a;
}
.tile-warning .tile-pct {
  color: #f59e0b;
}

.tile-danger {
  background: rgba(239, 68, 68, 0.15);
  border-color: rgba(239, 68, 68, 0.3);
}
.tile-danger:hover {
  background: rgba(239, 68, 68, 0.25);
  border-color: rgba(239, 68, 68, 0.5);
}
.tile-danger .tile-name {
  color: #fecaca;
}
.tile-danger .tile-pct {
  color: #ef4444;
}
</style>
