<template>
  <div class="detail" :class="side">
    <template v-if="throwData">
      <div class="detail-top">
        <span class="outcome" :class="throwData.is_hit ? 'hit' : 'miss'">
          {{ throwData.is_hit ? 'HIT' : 'MISS' }}
        </span>
        <span class="detail-time mono">{{ relTime }}</span>
      </div>
      <div class="detail-metrics">
        <div class="dm">
          <span class="hud-label">Peak</span>
          <span class="dm-val mono">{{ (throwData.peak_acceleration || 0).toFixed(2) }}<span class="u">g</span></span>
        </div>
        <div class="dm">
          <span class="hud-label">Drift</span>
          <span class="dm-val mono">{{ (throwData.drift_angle || 0).toFixed(1) }}<span class="u">°</span></span>
        </div>
      </div>
      <div class="detail-tags">
        <span class="tag" :class="catClass(throwData.control_category)">{{ throwData.control_category || '—' }}</span>
        <span class="tag" :class="catClass(throwData.drift_category)">{{ throwData.drift_category || '—' }}</span>
      </div>
    </template>
    <div v-else class="detail-empty mono">Noch keine Würfe</div>
  </div>
</template>

<script setup>
import { computed, ref, onMounted, onBeforeUnmount } from 'vue';
import { formatRelative } from '../services/time';

const props = defineProps({
  throwData: { type: Object, default: null },
  side: { type: String, default: 'a' },
});

const nowMs = ref(Date.now());
let clock = null;
onMounted(() => { clock = setInterval(() => { nowMs.value = Date.now(); }, 1000); });
onBeforeUnmount(() => clearInterval(clock));

const relTime = computed(() =>
  props.throwData && props.throwData.timestamp ? formatRelative(props.throwData.timestamp, nowMs.value) : 'gespeichert'
);

const catClass = (cat) => {
  if (cat === 'Controlled' || cat === 'Low Drift') return 'tag-good';
  if (cat === 'Moderately Controlled' || cat === 'Moderate Drift') return 'tag-mid';
  if (cat === 'Uncontrolled' || cat === 'Strong Drift') return 'tag-bad';
  return '';
};
</script>

<style scoped>
.detail {
  background: rgba(0, 0, 0, 0.3);
  border: 1px solid var(--border-color);
  border-radius: 12px;
  padding: 0.85rem 1rem;
  display: flex;
  flex-direction: column;
  gap: 0.6rem;
}
.detail.b { text-align: right; }

.detail-top {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.5rem;
}
.detail.b .detail-top { flex-direction: row-reverse; }
.outcome {
  font-family: var(--font-mono);
  font-size: 0.66rem;
  letter-spacing: 0.14em;
  padding: 3px 9px;
  border-radius: 5px;
}
.outcome.hit {
  color: var(--hit);
  background: rgba(47, 245, 184, 0.14);
  border: 1px solid rgba(47, 245, 184, 0.4);
}
.outcome.miss {
  color: var(--miss);
  background: rgba(255, 59, 107, 0.14);
  border: 1px solid rgba(255, 59, 107, 0.4);
}
.detail-time { font-size: 0.62rem; color: var(--text-dim); }

.detail-metrics {
  display: flex;
  gap: 1.5rem;
}
.detail.b .detail-metrics { flex-direction: row-reverse; }
.dm { display: flex; flex-direction: column; gap: 0.15rem; }
.dm-val { font-size: 1.15rem; font-weight: 600; color: var(--text-main); }
.dm-val .u { font-size: 0.7rem; color: var(--text-muted); margin-left: 1px; }

.detail-tags { display: flex; gap: 0.5rem; flex-wrap: wrap; }
.detail.b .detail-tags { justify-content: flex-end; }
.tag {
  font-size: 0.6rem;
  letter-spacing: 0.04em;
  padding: 3px 8px;
  border-radius: 5px;
  background: rgba(125, 170, 255, 0.08);
  border: 1px solid var(--border-color);
  color: var(--text-muted);
}
.tag-good { color: var(--hit); border-color: rgba(47, 245, 184, 0.3); }
.tag-mid { color: var(--warn); border-color: rgba(255, 182, 39, 0.3); }
.tag-bad { color: var(--miss); border-color: rgba(255, 59, 107, 0.3); }

.detail-empty {
  font-size: 0.72rem;
  color: var(--text-dim);
  text-align: center;
  padding: 0.6rem 0;
}
</style>
