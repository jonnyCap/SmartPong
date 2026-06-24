<template>
  <div class="player-select" :class="{ open: isOpen }" ref="root" :style="accentStyle">
    <button
      type="button"
      class="ps-trigger"
      :aria-expanded="isOpen"
      aria-haspopup="listbox"
      @click="toggle"
      @keydown="onTriggerKey"
    >
      <span class="ps-trigger-main">
        <span class="ps-dot" :class="{ live: selectedOnline }"></span>
        <span class="ps-name display">{{ selectedLabel }}</span>
      </span>
      <span class="ps-trigger-meta">
        <span class="ps-seen mono">{{ selectedSeen }}</span>
        <svg class="ps-caret" :class="{ flip: isOpen }" viewBox="0 0 24 24" width="14" height="14">
          <path d="M6 9l6 6 6-6" fill="none" stroke="currentColor" stroke-width="2.4"
                stroke-linecap="round" stroke-linejoin="round" />
        </svg>
      </span>
    </button>

    <transition name="ps-pop">
      <ul
        v-if="isOpen"
        class="ps-panel glass-panel"
        :class="direction"
        role="listbox"
        ref="panel"
      >
        <li v-if="users.length === 0" class="ps-empty mono">Warte auf Spielerdaten…</li>

        <li
          v-for="(u, i) in users"
          :key="u.user_id"
          role="option"
          :aria-selected="u.user_id === modelValue"
          class="ps-option"
          :class="{ selected: u.user_id === modelValue, active: i === activeIndex }"
          @click="choose(u.user_id)"
          @mousemove="activeIndex = i"
        >
          <span class="ps-opt-dot" :class="{ live: isOnline(u.last_seen, nowMs) }"></span>
          <span class="ps-opt-body">
            <span class="ps-opt-top">
              <span class="ps-opt-name display">{{ u.user_id }}</span>
              <span class="ps-opt-rate mono" v-if="u.total_throws > 0">
                {{ Math.round((u.hit_rate || 0) * 100) }}%
              </span>
            </span>
            <span class="ps-opt-bottom mono" :title="formatAbsolute(u.last_seen)">
              <span class="ps-opt-seen">{{ formatRelative(u.last_seen, nowMs) }}</span>
              <span class="ps-opt-sep">·</span>
              <span class="ps-opt-count">{{ u.total_throws || 0 }} Würfe</span>
            </span>
          </span>
          <span class="ps-opt-check" v-if="u.user_id === modelValue">◈</span>
        </li>
      </ul>
    </transition>
  </div>
</template>

<script setup>
import { ref, computed, watch, onMounted, onBeforeUnmount, nextTick } from 'vue';
import { formatRelative, formatAbsolute, isOnline } from '../services/time';

const props = defineProps({
  modelValue: { type: String, default: '' },
  users: { type: Array, default: () => [] },
  placeholder: { type: String, default: 'Spieler wählen' },
  direction: { type: String, default: 'down' }, // 'down' | 'up'
  accent: { type: String, default: '' },         // optional css color override
});

const emit = defineEmits(['update:modelValue']);

const root = ref(null);
const panel = ref(null);
const isOpen = ref(false);
const activeIndex = ref(-1);

// Live clock so relative timestamps tick without a re-fetch.
const nowMs = ref(Date.now());
let clock = null;

const accentStyle = computed(() =>
  props.accent ? { '--ps-accent': props.accent } : {}
);

const selected = computed(() =>
  props.users.find((u) => u.user_id === props.modelValue) || null
);
const selectedLabel = computed(() => props.modelValue || props.placeholder);
const selectedSeen = computed(() =>
  selected.value ? formatRelative(selected.value.last_seen, nowMs.value) : '—'
);
const selectedOnline = computed(() =>
  selected.value ? isOnline(selected.value.last_seen, nowMs.value) : false
);

const open = async () => {
  isOpen.value = true;
  activeIndex.value = Math.max(0, props.users.findIndex((u) => u.user_id === props.modelValue));
  await nextTick();
};
const close = () => {
  isOpen.value = false;
  activeIndex.value = -1;
};
const toggle = () => (isOpen.value ? close() : open());

const choose = (id) => {
  emit('update:modelValue', id);
  close();
};

const onTriggerKey = (e) => {
  if (e.key === 'ArrowDown' || e.key === 'Enter' || e.key === ' ') {
    e.preventDefault();
    if (!isOpen.value) open();
  } else if (e.key === 'Escape') {
    close();
  }
};

const onKeydown = (e) => {
  if (!isOpen.value) return;
  if (e.key === 'Escape') { close(); return; }
  if (e.key === 'ArrowDown') {
    e.preventDefault();
    activeIndex.value = Math.min(props.users.length - 1, activeIndex.value + 1);
  } else if (e.key === 'ArrowUp') {
    e.preventDefault();
    activeIndex.value = Math.max(0, activeIndex.value - 1);
  } else if (e.key === 'Enter') {
    e.preventDefault();
    const u = props.users[activeIndex.value];
    if (u) choose(u.user_id);
  }
};

const onClickOutside = (e) => {
  if (root.value && !root.value.contains(e.target)) close();
};

watch(() => props.users.length, () => {
  if (activeIndex.value >= props.users.length) activeIndex.value = props.users.length - 1;
});

onMounted(() => {
  clock = setInterval(() => { nowMs.value = Date.now(); }, 1000);
  document.addEventListener('click', onClickOutside, true);
  document.addEventListener('keydown', onKeydown);
});

onBeforeUnmount(() => {
  clearInterval(clock);
  document.removeEventListener('click', onClickOutside, true);
  document.removeEventListener('keydown', onKeydown);
});
</script>

<style scoped>
.player-select {
  --ps-accent: var(--accent-color);
  position: relative;
  width: 100%;
}

/* ---- Trigger ---- */
.ps-trigger {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.6rem;
  padding: 0.7rem 0.85rem;
  border-radius: 12px;
  cursor: pointer;
  color: var(--text-main);
  background:
    linear-gradient(150deg, rgba(255, 255, 255, 0.04), rgba(0, 0, 0, 0.25));
  border: 1px solid var(--border-color);
  transition: border-color 0.2s, box-shadow 0.2s, transform 0.1s;
  font-family: inherit;
}
.ps-trigger:hover {
  border-color: color-mix(in srgb, var(--ps-accent) 55%, transparent);
  box-shadow: 0 0 0 1px color-mix(in srgb, var(--ps-accent) 30%, transparent),
              0 8px 24px -14px color-mix(in srgb, var(--ps-accent) 80%, transparent);
}
.player-select.open .ps-trigger {
  border-color: color-mix(in srgb, var(--ps-accent) 70%, transparent);
  box-shadow: 0 0 18px -4px color-mix(in srgb, var(--ps-accent) 75%, transparent);
}

.ps-trigger-main {
  display: flex;
  align-items: center;
  gap: 0.55rem;
  min-width: 0;
}
.ps-name {
  font-size: 0.95rem;
  font-weight: 600;
  letter-spacing: 0.03em;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.ps-trigger-meta {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  flex-shrink: 0;
}
.ps-seen {
  font-size: 0.66rem;
  color: var(--text-dim);
}
.ps-caret {
  color: var(--text-muted);
  transition: transform 0.25s ease;
}
.ps-caret.flip { transform: rotate(180deg); color: var(--ps-accent); }

/* ---- Live dot ---- */
.ps-dot, .ps-opt-dot {
  width: 9px;
  height: 9px;
  border-radius: 50%;
  flex-shrink: 0;
  background: var(--text-dim);
  position: relative;
}
.ps-dot.live, .ps-opt-dot.live {
  background: var(--hit);
  box-shadow: 0 0 8px var(--hit-glow);
  animation: ringPulse 2s infinite;
}

/* ---- Panel ---- */
.ps-panel {
  position: absolute;
  left: 0;
  right: 0;
  z-index: 60;
  list-style: none;
  padding: 0.4rem;
  max-height: 320px;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 2px;
  border-color: color-mix(in srgb, var(--ps-accent) 35%, var(--border-color));
}
.ps-panel.down { top: calc(100% + 8px); }
.ps-panel.up { bottom: calc(100% + 8px); }

.ps-empty {
  padding: 1rem 0.75rem;
  font-size: 0.78rem;
  color: var(--text-dim);
  text-align: center;
}

.ps-option {
  display: flex;
  align-items: center;
  gap: 0.7rem;
  padding: 0.6rem 0.7rem;
  border-radius: 10px;
  cursor: pointer;
  border: 1px solid transparent;
  transition: background 0.12s, border-color 0.12s;
}
.ps-option.active {
  background: color-mix(in srgb, var(--ps-accent) 12%, transparent);
  border-color: color-mix(in srgb, var(--ps-accent) 30%, transparent);
}
.ps-option.selected {
  background: color-mix(in srgb, var(--ps-accent) 16%, transparent);
}

.ps-opt-body {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.ps-opt-top {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 0.5rem;
}
.ps-opt-name {
  font-size: 0.9rem;
  font-weight: 600;
  letter-spacing: 0.02em;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.ps-opt-rate {
  font-size: 0.7rem;
  color: var(--ps-accent);
  flex-shrink: 0;
}
.ps-opt-bottom {
  font-size: 0.64rem;
  color: var(--text-dim);
  display: flex;
  align-items: center;
  gap: 0.35rem;
}
.ps-opt-sep { opacity: 0.5; }
.ps-opt-check {
  color: var(--ps-accent);
  font-size: 0.8rem;
  text-shadow: 0 0 10px color-mix(in srgb, var(--ps-accent) 70%, transparent);
}

/* ---- Pop transition ---- */
.ps-pop-enter-active, .ps-pop-leave-active {
  transition: opacity 0.16s ease, transform 0.16s ease;
}
.ps-pop-enter-from, .ps-pop-leave-to {
  opacity: 0;
  transform: translateY(-6px) scale(0.98);
}
.ps-panel.up.ps-pop-enter-from, .ps-panel.up.ps-pop-leave-to {
  transform: translateY(6px) scale(0.98);
}
</style>
