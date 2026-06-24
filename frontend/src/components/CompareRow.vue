<template>
  <div class="cmp">
    <div class="cmp-vals">
      <span class="val val-a mono" :class="{ win: winner === 'a' }">{{ fmt(a) }}{{ suffix }}</span>
      <span class="cmp-label hud-label">{{ label }}</span>
      <span class="val val-b mono" :class="{ win: winner === 'b' }">{{ fmt(b) }}{{ suffix }}</span>
    </div>
    <div class="cmp-bars">
      <div class="track left">
        <div class="fill fill-a" :class="{ win: winner === 'a' }" :style="{ width: aFill + '%' }"></div>
      </div>
      <div class="node" :class="winner"></div>
      <div class="track right">
        <div class="fill fill-b" :class="{ win: winner === 'b' }" :style="{ width: bFill + '%' }"></div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue';

const props = defineProps({
  label: { type: String, required: true },
  a: { type: Number, default: 0 },
  b: { type: Number, default: 0 },
  suffix: { type: String, default: '' },
  decimals: { type: Number, default: 0 },
  higherWins: { type: Boolean, default: false },
});

const fmt = (v) => (v || 0).toFixed(props.decimals);

const maxBoth = computed(() => Math.max(props.a, props.b, 0.0001));
const aFill = computed(() => (props.a / maxBoth.value) * 100);
const bFill = computed(() => (props.b / maxBoth.value) * 100);

const winner = computed(() => {
  if (props.a === props.b) return 'tie';
  if (props.higherWins) return props.a > props.b ? 'a' : 'b';
  return props.a < props.b ? 'a' : 'b';
});
</script>

<style scoped>
.cmp { display: flex; flex-direction: column; gap: 0.35rem; }
.cmp-vals {
  display: grid;
  grid-template-columns: 1fr auto 1fr;
  align-items: center;
  gap: 0.5rem;
}
.val { font-size: 0.85rem; font-weight: 600; color: var(--text-muted); }
.val-a { text-align: left; }
.val-b { text-align: right; }
.val-a.win { color: var(--player-a); text-shadow: 0 0 12px rgba(var(--player-a-rgb), 0.6); }
.val-b.win { color: var(--player-b); text-shadow: 0 0 12px rgba(var(--player-b-rgb), 0.6); }
.cmp-label {
  font-size: 0.55rem;
  letter-spacing: 0.12em;
  text-align: center;
  white-space: nowrap;
}

.cmp-bars {
  display: flex;
  align-items: center;
  gap: 0;
}
.track {
  flex: 1;
  height: 6px;
  background: rgba(0, 0, 0, 0.45);
  border-radius: 4px;
  overflow: hidden;
  display: flex;
}
.track.left { justify-content: flex-end; }
.track.right { justify-content: flex-start; }
.fill {
  height: 100%;
  border-radius: 4px;
  transition: width 0.6s cubic-bezier(0.22, 1, 0.36, 1);
  opacity: 0.55;
}
.fill-a { background: var(--player-a); }
.fill-b { background: var(--player-b); }
.fill-a.win { opacity: 1; box-shadow: 0 0 12px rgba(var(--player-a-rgb), 0.7); }
.fill-b.win { opacity: 1; box-shadow: 0 0 12px rgba(var(--player-b-rgb), 0.7); }

.node {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  margin: 0 4px;
  background: var(--text-dim);
  flex-shrink: 0;
  transition: background 0.3s, box-shadow 0.3s;
}
.node.a { background: var(--player-a); box-shadow: 0 0 8px var(--player-a); }
.node.b { background: var(--player-b); box-shadow: 0 0 8px var(--player-b); }
</style>
