<template>
  <div class="qmeter">
    <div class="qmeter-head">
      <span class="hud-label">{{ label }}</span>
      <span class="qmeter-dom mono" :class="'tone-' + dominant.tone">{{ dominant.label }}</span>
    </div>

    <div class="qbar">
      <div
        v-for="s in segments"
        :key="s.label"
        class="qseg"
        :class="['tone-' + s.tone, { active: s.label === dominant.label }]"
        :style="{ width: s.pct }"
      ></div>
    </div>

    <div class="qlegend">
      <span
        v-for="s in segments"
        :key="s.label"
        class="qleg"
        :class="['tone-' + s.tone, { active: s.label === dominant.label }]"
      >
        <i></i>{{ s.label }} <b class="mono">{{ s.pct }}</b>
      </span>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue';

const props = defineProps({
  label: { type: String, required: true },
  // [{ label, pct: "12.5%", tone: 'good' | 'mid' | 'bad' }]
  segments: { type: Array, required: true },
});

const num = (s) => parseFloat(String(s).replace('%', '')) || 0;

const dominant = computed(() =>
  props.segments.reduce((best, s) => (num(s.pct) >= num(best.pct) ? s : best), props.segments[0])
);
</script>

<style scoped>
.qmeter {
  display: flex;
  flex-direction: column;
  gap: 0.6rem;
}

.qmeter-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.qmeter-dom {
  font-size: 0.72rem;
  font-weight: 600;
  padding: 3px 10px;
  border-radius: 6px;
  letter-spacing: 0.04em;
}

/* ---- Segmented bar ---- */
.qbar {
  display: flex;
  height: 12px;
  border-radius: 7px;
  overflow: hidden;
  background: rgba(0, 0, 0, 0.45);
  border: 1px solid var(--border-color);
}
.qseg {
  height: 100%;
  transition: width 0.6s cubic-bezier(0.22, 1, 0.36, 1), opacity 0.3s;
  opacity: 0.4;
}
.qseg.active { opacity: 1; }

/* ---- Legend ---- */
.qlegend {
  display: flex;
  flex-wrap: wrap;
  gap: 0.4rem 1.1rem;
  font-size: 0.66rem;
  color: var(--text-muted);
}
.qleg {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}
.qleg i {
  width: 9px;
  height: 9px;
  border-radius: 3px;
  flex-shrink: 0;
  opacity: 0.5;
}
.qleg.active { color: var(--text-main); }
.qleg.active i { opacity: 1; }
.qleg b { color: var(--text-main); font-size: 0.66rem; }

/* ---- Tones ---- */
.tone-good { color: var(--hit); }
.tone-mid { color: var(--warn); }
.tone-bad { color: var(--miss); }

.qseg.tone-good { background: var(--hit); }
.qseg.tone-mid { background: var(--warn); }
.qseg.tone-bad { background: var(--miss); }
.qseg.active.tone-good { box-shadow: inset 0 0 12px rgba(47, 245, 184, 0.6); }
.qseg.active.tone-mid { box-shadow: inset 0 0 12px rgba(255, 182, 39, 0.6); }
.qseg.active.tone-bad { box-shadow: inset 0 0 12px rgba(255, 59, 107, 0.6); }

.qleg.tone-good i { background: var(--hit); }
.qleg.tone-mid i { background: var(--warn); }
.qleg.tone-bad i { background: var(--miss); }

.qmeter-dom.tone-good { background: rgba(47, 245, 184, 0.14); border: 1px solid rgba(47, 245, 184, 0.4); }
.qmeter-dom.tone-mid { background: rgba(255, 182, 39, 0.14); border: 1px solid rgba(255, 182, 39, 0.4); }
.qmeter-dom.tone-bad { background: rgba(255, 59, 107, 0.14); border: 1px solid rgba(255, 59, 107, 0.4); }
</style>
