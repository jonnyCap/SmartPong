<template>
  <div class="dashboard-wrapper glass-panel hud-corners">
    <div class="dash-head">
      <span class="hud-label">Profile Telemetry</span>
      <span class="view-chip mono">{{ viewModeLabel }}</span>
    </div>

    <div v-if="userProfile && filteredThrows.length > 0" class="dash-body">
      <!-- Hero: Hit rate -->
      <div class="hero neon-frame">
        <div class="hero-left">
          <span class="hud-label">Hit Rate</span>
          <div class="hero-value display">
            {{ hitRate.toFixed(1) }}<span class="unit">%</span>
          </div>
          <div class="hero-bar">
            <div class="hero-bar-fill" :style="{ width: hitRate + '%' }"></div>
          </div>
        </div>
        <div class="hero-right">
          <div class="mini-stat">
            <span class="hud-label">Throws</span>
            <span class="mini-val display">{{ filteredThrows.length }}</span>
          </div>
          <div class="mini-stat">
            <span class="hud-label">Hits</span>
            <span class="mini-val display glow-hit">{{ hitCount }}</span>
          </div>
        </div>
      </div>

      <!-- Stat cards -->
      <div class="stats-grid">
        <div class="stat-card">
          <span class="hud-label">Peak Accel</span>
          <span class="value display glow-cyan">{{ avgAccel.toFixed(2) }}<span class="unit"> g</span></span>
          <span class="subtext mono">min {{ minAccel.toFixed(2) }} · max {{ maxAccel.toFixed(2) }}</span>
        </div>
        <div class="stat-card">
          <span class="hud-label">Drift Angle</span>
          <span class="value display glow-magenta">{{ avgDrift.toFixed(1) }}<span class="unit">°</span></span>
          <span class="subtext mono">min {{ minDrift.toFixed(1) }}° · max {{ maxDrift.toFixed(1) }}°</span>
        </div>
      </div>
    </div>
    <div v-else-if="userProfile" class="loading">No throws for this selection.</div>
    <div v-else class="loading">Loading profile data…</div>

    <div v-if="userProfile && filteredThrows.length > 0" class="quality-block">
      <QualityMeter label="Control Quality" :segments="controlSegments" />
      <QualityMeter label="Drift Quality" :segments="driftSegments" />
    </div>
  </div>
</template>

<script setup>
import { defineProps, computed } from 'vue';
import { peakOf, driftOf, controlOf, driftCatOf } from '../services/prefs';
import QualityMeter from './QualityMeter.vue';

const props = defineProps({
  userProfile: { type: Object, default: null },
  latestThrow: { type: Object, default: null },
  viewMode: { type: String, default: 'all' },
});

const viewModeLabel = computed(() => {
  if (props.viewMode === 'all') return 'ALL THROWS';
  if (props.viewMode === 'hits') return 'HITS ONLY';
  if (props.viewMode === 'misses') return 'MISSES ONLY';
  if (props.viewMode === 'latest') return 'LATEST THROW';
  if (props.viewMode.startsWith('history-')) {
    const idx = parseInt(props.viewMode.split('-')[1]);
    const total = props.userProfile?.throws?.length || 0;
    return `THROW #${total - idx}`;
  }
  return 'SELECTED';
});

const filteredThrows = computed(() => {
  if (!props.userProfile || !props.userProfile.throws) return [];
  if (props.viewMode === 'all') return props.userProfile.throws;
  if (props.viewMode === 'hits') return props.userProfile.throws.filter((t) => t.is_hit);
  if (props.viewMode === 'misses') return props.userProfile.throws.filter((t) => !t.is_hit);
  if (props.viewMode === 'latest') {
    if (props.latestThrow) return [props.latestThrow];
    const len = props.userProfile.throws.length;
    return len > 0 ? [props.userProfile.throws[len - 1]] : [];
  }
  if (props.viewMode.startsWith('history-')) {
    const idx = parseInt(props.viewMode.split('-')[1]);
    return props.userProfile.throws[idx] ? [props.userProfile.throws[idx]] : [];
  }
  return [];
});

const hitCount = computed(() => filteredThrows.value.filter((t) => t.is_hit || t.hit).length);

const hitRate = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return (hitCount.value / filteredThrows.value.length) * 100;
});

const avgAccel = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return filteredThrows.value.reduce((sum, t) => sum + peakOf(t), 0) / filteredThrows.value.length;
});
const minAccel = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return Math.min(...filteredThrows.value.map((t) => peakOf(t)));
});
const maxAccel = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return Math.max(...filteredThrows.value.map((t) => peakOf(t)));
});

const avgDrift = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return filteredThrows.value.reduce((sum, t) => sum + driftOf(t), 0) / filteredThrows.value.length;
});
const minDrift = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return Math.min(...filteredThrows.value.map((t) => driftOf(t)));
});
const maxDrift = computed(() => {
  if (filteredThrows.value.length === 0) return 0;
  return Math.max(...filteredThrows.value.map((t) => driftOf(t)));
});

const classPercentages = computed(() => {
  const throws = filteredThrows.value;
  const count = throws.length;

  const control = { 'Controlled': 0, 'Moderately Controlled': 0, 'Uncontrolled': 0 };
  const drift = { 'Low Drift': 0, 'Moderate Drift': 0, 'Strong Drift': 0 };

  if (count === 0) {
    Object.keys(control).forEach((k) => (control[k] = '0.0%'));
    Object.keys(drift).forEach((k) => (drift[k] = '0.0%'));
    return { control, drift };
  }

  throws.forEach((t) => {
    const c = controlOf(t);
    const d = driftCatOf(t);
    if (c) control[c] = (control[c] || 0) + 1;
    if (d) drift[d] = (drift[d] || 0) + 1;
  });

  Object.keys(control).forEach((k) => (control[k] = ((control[k] / count) * 100).toFixed(1) + '%'));
  Object.keys(drift).forEach((k) => (drift[k] = ((drift[k] / count) * 100).toFixed(1) + '%'));

  return { control, drift };
});

const controlSegments = computed(() => [
  { label: 'Controlled', pct: classPercentages.value.control['Controlled'], tone: 'good' },
  { label: 'Moderate', pct: classPercentages.value.control['Moderately Controlled'], tone: 'mid' },
  { label: 'Uncontrolled', pct: classPercentages.value.control['Uncontrolled'], tone: 'bad' },
]);

const driftSegments = computed(() => [
  { label: 'Low', pct: classPercentages.value.drift['Low Drift'], tone: 'good' },
  { label: 'Moderate', pct: classPercentages.value.drift['Moderate Drift'], tone: 'mid' },
  { label: 'Strong', pct: classPercentages.value.drift['Strong Drift'], tone: 'bad' },
]);
</script>

<style scoped>
.dashboard-wrapper {
  padding: 1.4rem;
  display: flex;
  flex-direction: column;
  gap: 1.2rem;
  overflow-y: auto;
}

.dash-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.view-chip {
  font-size: 0.65rem;
  padding: 4px 10px;
  border-radius: 6px;
  color: var(--accent-color);
  background: rgba(var(--accent-rgb), 0.1);
  border: 1px solid color-mix(in srgb, var(--accent-color) 35%, transparent);
}

.dash-body {
  display: flex;
  flex-direction: column;
  gap: 1.2rem;
}

/* ---- Hero ---- */
.hero {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  padding: 1.3rem 1.4rem;
  border-radius: 14px;
  background:
    radial-gradient(120% 140% at 0% 0%, rgba(var(--accent-rgb), 0.14), transparent 60%),
    linear-gradient(150deg, rgba(255, 255, 255, 0.03), rgba(0, 0, 0, 0.3));
}
.hero-left { display: flex; flex-direction: column; gap: 0.4rem; flex: 1; }
.hero-value {
  font-size: 3rem;
  font-weight: 800;
  line-height: 1;
  color: var(--text-main);
  text-shadow: 0 0 24px rgba(var(--accent-rgb), 0.4);
}
.hero-value .unit { font-size: 1.2rem; color: var(--text-muted); margin-left: 2px; }
.hero-bar {
  margin-top: 0.5rem;
  height: 7px;
  border-radius: 4px;
  background: rgba(0, 0, 0, 0.45);
  overflow: hidden;
  position: relative;
}
.hero-bar-fill {
  height: 100%;
  border-radius: 4px;
  background: linear-gradient(90deg, var(--accent-color), var(--accent-2));
  box-shadow: 0 0 14px var(--accent-color);
  transition: width 0.6s cubic-bezier(0.22, 1, 0.36, 1);
}
.hero-right {
  display: flex;
  flex-direction: column;
  justify-content: center;
  gap: 0.9rem;
  padding-left: 1rem;
  border-left: 1px solid var(--border-color);
}
.mini-stat { display: flex; flex-direction: column; gap: 0.2rem; }
.mini-val { font-size: 1.5rem; font-weight: 700; }

/* ---- Stat cards ---- */
.stats-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1rem;
}
.stat-card {
  position: relative;
  background: linear-gradient(160deg, rgba(255, 255, 255, 0.03), rgba(0, 0, 0, 0.28));
  padding: 1rem 1.1rem;
  border-radius: 12px;
  display: flex;
  flex-direction: column;
  gap: 0.4rem;
  border: 1px solid var(--border-color);
  overflow: hidden;
  transition: transform 0.25s ease, border-color 0.25s ease;
}
.stat-card::before {
  content: "";
  position: absolute;
  top: 0; left: 0; right: 0;
  height: 2px;
  background: linear-gradient(90deg, transparent, var(--accent-color), transparent);
  opacity: 0.6;
}
.stat-card:hover {
  transform: translateY(-3px);
  border-color: var(--border-bright);
}
.value { font-size: 1.7rem; font-weight: 700; line-height: 1.1; }
.value .unit { font-size: 0.9rem; color: var(--text-muted); }
.subtext { font-size: 0.66rem; color: var(--text-dim); }

/* ---- Quality ---- */
.quality-block { display: flex; flex-direction: column; gap: 1.4rem; margin-top: 0.2rem; }

.loading {
  color: var(--text-dim);
  font-style: italic;
  font-size: 0.85rem;
  padding: 1.5rem 0;
  text-align: center;
}

</style>
