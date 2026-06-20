<template>
  <div class="dashboard-wrapper glass-panel">
    <h3>Profile Stats</h3>
    
    <div v-if="userProfile" class="stats-grid">
      <div class="stat-card">
        <span class="label">Total Throws</span>
        <span class="value">{{ userProfile.total_throws }}</span>
      </div>
      <div class="stat-card">
        <span class="label">Hit Rate</span>
        <span class="value">{{ (userProfile.hit_rate * 100).toFixed(1) }}%</span>
      </div>
      <div class="stat-card">
        <span class="label">Avg Peak Accel</span>
        <span class="value">{{ userProfile.avg_peak_acceleration?.toFixed(2) || '0.00' }} g</span>
      </div>
      <div class="stat-card">
        <span class="label">Avg Drift Angle</span>
        <span class="value">{{ userProfile.avg_drift_angle?.toFixed(1) || '0.0' }}°</span>
      </div>
    </div>
    <div v-else class="loading">Loading profile data...</div>

    <h3 class="mt-4">Latest Throw</h3>
    <div v-if="latestThrow" class="latest-throw">
      <div class="throw-detail" :class="latestThrow.control_category === 'Controlled' ? 'good' : 'warning'">
        <span class="label">Control</span>
        <span class="value">{{ latestThrow.control_category }}</span>
        <span class="subtext">({{ latestThrow.peak_acceleration?.toFixed(2) }} g)</span>
      </div>
      
      <div class="throw-detail" :class="latestThrow.drift_category === 'Low Drift' ? 'good' : 'warning'">
        <span class="label">Drift</span>
        <span class="value">{{ latestThrow.drift_category }}</span>
        <span class="subtext">({{ latestThrow.drift_angle?.toFixed(1) }}°)</span>
      </div>
    </div>
    <div v-else class="loading">Waiting for next throw...</div>
  </div>
</template>

<script setup>
import { defineProps } from 'vue';

const props = defineProps({
  userProfile: {
    type: Object,
    default: null
  },
  latestThrow: {
    type: Object,
    default: null
  }
});
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

.latest-throw {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.throw-detail {
  background: rgba(0, 0, 0, 0.2);
  padding: 1rem 1.5rem;
  border-radius: 12px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  border-left: 4px solid var(--border-color);
  animation: slideIn 0.3s ease-out;
}

@keyframes slideIn {
  from { opacity: 0; transform: translateX(-10px); }
  to { opacity: 1; transform: translateX(0); }
}

.throw-detail.good {
  border-left-color: var(--success-color);
  background: rgba(16, 185, 129, 0.05);
}

.throw-detail.warning {
  border-left-color: var(--danger-color);
  background: rgba(239, 68, 68, 0.05);
}

.throw-detail .label {
  flex: 1;
}

.throw-detail .value {
  font-size: 1.1rem;
  font-weight: 600;
  margin-right: 1rem;
}

.throw-detail .subtext {
  font-size: 0.85rem;
  color: var(--text-muted);
  width: 60px;
  text-align: right;
}

.loading {
  color: var(--text-muted);
  font-style: italic;
  font-size: 0.9rem;
  padding: 1rem 0;
}
</style>
