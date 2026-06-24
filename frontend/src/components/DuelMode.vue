<template>
  <div class="duel">
    <!-- Top selector bar -->
    <div class="duel-pickers">
      <div class="picker picker-a">
        <span class="hud-label">Player A</span>
        <PlayerSelect v-model="playerA" :users="users" accent="var(--player-a)" placeholder="Spieler A" />
      </div>

      <div class="vs-col">
        <div class="vs-badge">
          <span class="vs-text display">VS</span>
        </div>
        <button
          class="reset-btn"
          :class="resetPhase"
          :disabled="resetPhase === 'working'"
          :title="'Setzt die Stats beider Spieler serverseitig zurück'"
          @click="onResetClick"
        >
          <span class="reset-ico">⟲</span>
          <span class="reset-lbl">{{ resetLabel }}</span>
        </button>
      </div>

      <div class="picker picker-b">
        <span class="hud-label">Player B</span>
        <PlayerSelect v-model="playerB" :users="users" accent="var(--player-b)" placeholder="Spieler B" />
      </div>
    </div>

    <!-- Arena -->
    <div class="arena">
      <!-- Side A -->
      <section class="side side-a glass-panel hud-corners" :class="{ flash: flashA, lead: leadCount.a > leadCount.b }">
        <div class="side-head">
          <span class="side-dot"></span>
          <span class="side-name display">{{ playerA || '—' }}</span>
          <transition name="ping"><span v-if="flashA" class="new-ping mono">NEW THROW</span></transition>
        </div>

        <div class="side-stats">
          <div class="side-hero">
            <span class="hud-label">Hit Rate</span>
            <div class="side-hero-val display">{{ statsA.hitRate.toFixed(0) }}<span class="u">%</span></div>
          </div>
          <div class="side-tally">
            <div class="tally tally-hit">
              <span class="tally-num display">{{ statsA.hits }}</span>
              <span class="tally-lbl mono">HITS</span>
            </div>
            <div class="tally tally-miss">
              <span class="tally-num display">{{ statsA.misses }}</span>
              <span class="tally-lbl mono">MISS</span>
            </div>
          </div>
        </div>

        <Throw3DViewer
          class="side-viewer"
          compact
          :user-profile="profileA"
          :latest-trajectory="latestA?.trajectory"
          :view-mode="viewerModeA"
          accent-color="#22d3ee"
          force-color="#22d3ee"
        />

        <ThrowDetail :throw-data="newestA" side="a" />
      </section>

      <!-- Center comparison -->
      <div class="versus">
        <CompareRow label="Hit Rate" :a="statsA.hitRate" :b="statsB.hitRate" suffix="%" higher-wins />
        <CompareRow label="Peak Accel" :a="statsA.avgAccel" :b="statsB.avgAccel" suffix=" g" :decimals="2" higher-wins />
        <CompareRow label="Drift" :a="statsA.avgDrift" :b="statsB.avgDrift" suffix="°" :decimals="1" />
        <CompareRow label="Throws" :a="statsA.count" :b="statsB.count" higher-wins />

        <div class="scoreline">
          <span class="score-a display" :class="{ win: leadCount.a > leadCount.b }">{{ leadCount.a }}</span>
          <span class="score-sep">:</span>
          <span class="score-b display" :class="{ win: leadCount.b > leadCount.a }">{{ leadCount.b }}</span>
        </div>
        <span class="hud-label score-label">Categories Won</span>
      </div>

      <!-- Side B -->
      <section class="side side-b glass-panel hud-corners" :class="{ flash: flashB, lead: leadCount.b > leadCount.a }">
        <div class="side-head">
          <span class="side-dot"></span>
          <span class="side-name display">{{ playerB || '—' }}</span>
          <transition name="ping"><span v-if="flashB" class="new-ping mono">NEW THROW</span></transition>
        </div>

        <div class="side-stats">
          <div class="side-hero">
            <span class="hud-label">Hit Rate</span>
            <div class="side-hero-val display">{{ statsB.hitRate.toFixed(0) }}<span class="u">%</span></div>
          </div>
          <div class="side-tally">
            <div class="tally tally-hit">
              <span class="tally-num display">{{ statsB.hits }}</span>
              <span class="tally-lbl mono">HITS</span>
            </div>
            <div class="tally tally-miss">
              <span class="tally-num display">{{ statsB.misses }}</span>
              <span class="tally-lbl mono">MISS</span>
            </div>
          </div>
        </div>

        <Throw3DViewer
          class="side-viewer"
          compact
          :user-profile="profileB"
          :latest-trajectory="latestB?.trajectory"
          :view-mode="viewerModeB"
          accent-color="#ff36d7"
          force-color="#ff36d7"
        />

        <ThrowDetail :throw-data="newestB" side="b" />
      </section>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, watch, onMounted, onBeforeUnmount } from 'vue';
import PlayerSelect from './PlayerSelect.vue';
import Throw3DViewer from './Throw3DViewer.vue';
import ThrowDetail from './ThrowDetail.vue';
import CompareRow from './CompareRow.vue';
import { fetchUserProfile, resetProfile } from '../services/api';
import { connectMqtt, onResult } from '../services/mqttClient';

const props = defineProps({
  users: { type: Array, default: () => [] },
});

const playerA = ref('');
const playerB = ref('');
const profileA = ref(null);
const profileB = ref(null);
const latestA = ref(null);
const latestB = ref(null);
const flashA = ref(false);
const flashB = ref(false);

let unsubResult = null;
let flashTimerA = null;
let flashTimerB = null;

const emptyStats = { hitRate: 0, avgAccel: 0, avgDrift: 0, count: 0, hits: 0, misses: 0 };

const computeStats = (profile) => {
  if (!profile || !profile.throws || profile.throws.length === 0) return { ...emptyStats };
  const throws = profile.throws;
  const count = throws.length;
  const hits = throws.filter((t) => t.is_hit).length;
  const avgAccel = throws.reduce((s, t) => s + (t.peak_acceleration || 0), 0) / count;
  const avgDrift = throws.reduce((s, t) => s + (t.drift_angle || 0), 0) / count;
  return { hitRate: (hits / count) * 100, avgAccel, avgDrift, count, hits, misses: count - hits };
};

const statsA = computed(() => computeStats(profileA.value));
const statsB = computed(() => computeStats(profileB.value));

// Most recent throw to show in the detail card: live throw wins, else last stored.
const lastStored = (profile) => {
  if (!profile || !profile.throws || profile.throws.length === 0) return null;
  return profile.throws[profile.throws.length - 1];
};
const newestA = computed(() => latestA.value || lastStored(profileA.value));
const newestB = computed(() => latestB.value || lastStored(profileB.value));

// Viewer mode: show live 'latest', else the last stored throw's path.
const viewerModeFor = (latest, profile) => {
  if (latest) return 'latest';
  if (profile && profile.throws && profile.throws.length > 0) {
    return 'history-' + (profile.throws.length - 1);
  }
  return 'latest';
};
const viewerModeA = computed(() => viewerModeFor(latestA.value, profileA.value));
const viewerModeB = computed(() => viewerModeFor(latestB.value, profileB.value));

// Count how many metric categories each side leads (drift: lower wins).
const leadCount = computed(() => {
  const a = statsA.value, b = statsB.value;
  let ac = 0, bc = 0;
  const higher = (x, y) => { if (x > y) ac++; else if (y > x) bc++; };
  const lower = (x, y) => { if (x < y) ac++; else if (y < x) bc++; };
  higher(a.hitRate, b.hitRate);
  higher(a.avgAccel, b.avgAccel);
  lower(a.avgDrift, b.avgDrift);
  higher(a.count, b.count);
  return { a: ac, b: bc };
});

const loadA = async () => { profileA.value = playerA.value ? await fetchUserProfile(playerA.value) : null; };
const loadB = async () => { profileB.value = playerB.value ? await fetchUserProfile(playerB.value) : null; };

watch(playerA, () => { latestA.value = null; loadA(); });
watch(playerB, () => { latestB.value = null; loadB(); });

// Seed default players A/B from the two most-recently-active users.
watch(() => props.users, (list) => {
  if (!list || list.length === 0) return;
  const ids = list.map((u) => u.user_id);
  if (!playerA.value || !ids.includes(playerA.value)) playerA.value = ids[0];
  if ((!playerB.value || !ids.includes(playerB.value)) && ids.length > 1) {
    playerB.value = ids.find((id) => id !== playerA.value) || ids[0];
  }
}, { immediate: true });

// --- Server-side reset (both players), with a two-step confirm ---
const resetPhase = ref('idle'); // idle | confirm | working
let confirmTimer = null;

const resetLabel = computed(() => {
  if (resetPhase.value === 'working') return 'Reset…';
  if (resetPhase.value === 'confirm') return 'Wirklich? Bestätigen';
  return 'Reset Stats';
});

const onResetClick = async () => {
  if (resetPhase.value === 'working') return;

  if (resetPhase.value === 'idle') {
    resetPhase.value = 'confirm';
    clearTimeout(confirmTimer);
    confirmTimer = setTimeout(() => {
      if (resetPhase.value === 'confirm') resetPhase.value = 'idle';
    }, 4000);
    return;
  }

  // confirm -> execute
  clearTimeout(confirmTimer);
  resetPhase.value = 'working';
  const targets = [...new Set([playerA.value, playerB.value].filter(Boolean))];
  await Promise.all(targets.map((id) => resetProfile(id)));
  latestA.value = null;
  latestB.value = null;
  await Promise.all([loadA(), loadB()]);
  resetPhase.value = 'idle';
};

const triggerFlash = (side) => {
  if (side === 'a') {
    flashA.value = true;
    clearTimeout(flashTimerA);
    flashTimerA = setTimeout(() => { flashA.value = false; }, 1400);
  } else {
    flashB.value = true;
    clearTimeout(flashTimerB);
    flashTimerB = setTimeout(() => { flashB.value = false; }, 1400);
  }
};

onMounted(() => {
  connectMqtt();
  unsubResult = onResult((payload) => {
    if (payload.user_id === playerA.value) {
      latestA.value = payload;
      loadA();
      triggerFlash('a');
    }
    if (payload.user_id === playerB.value) {
      latestB.value = payload;
      loadB();
      triggerFlash('b');
    }
  });
});

onBeforeUnmount(() => {
  if (unsubResult) unsubResult();
  clearTimeout(flashTimerA);
  clearTimeout(flashTimerB);
  clearTimeout(confirmTimer);
});
</script>

<style scoped>
.duel {
  display: flex;
  flex-direction: column;
  gap: 1.1rem;
  flex-grow: 1;
  min-height: 0;
}

/* ---- Pickers ---- */
.duel-pickers {
  display: grid;
  grid-template-columns: 1fr auto 1fr;
  align-items: end;
  gap: 1.5rem;
}
.picker { display: flex; flex-direction: column; gap: 0.4rem; }
.picker-a { --accent-color: var(--player-a); }
.picker-b { --accent-color: var(--player-b); }
.vs-col {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.6rem;
}
.vs-badge {
  width: 54px;
  height: 54px;
  border-radius: 50%;
  display: grid;
  place-items: center;
  background: radial-gradient(circle, rgba(154, 107, 255, 0.25), rgba(4, 6, 14, 0.6));
  border: 1.5px solid rgba(154, 107, 255, 0.5);
  box-shadow: 0 0 24px -6px var(--neon-violet);
}
.vs-text { font-weight: 900; font-size: 1.2rem; color: #fff; text-shadow: 0 0 12px var(--neon-violet); }

.reset-btn {
  display: flex;
  align-items: center;
  gap: 0.4rem;
  padding: 0.45rem 0.8rem;
  border-radius: 8px;
  font-family: var(--font-mono);
  font-size: 0.62rem;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  white-space: nowrap;
  cursor: pointer;
  color: var(--text-muted);
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid var(--border-color);
  transition: all 0.2s ease;
}
.reset-btn:hover {
  color: var(--miss);
  border-color: rgba(255, 59, 107, 0.45);
  background: rgba(255, 59, 107, 0.08);
}
.reset-btn .reset-ico { font-size: 0.85rem; line-height: 1; }
.reset-btn.confirm {
  color: #fff;
  background: linear-gradient(135deg, var(--miss), #c81e4a);
  border-color: var(--miss);
  box-shadow: 0 0 18px -4px var(--miss-glow);
  animation: resetPulse 1s ease infinite;
}
.reset-btn.working { opacity: 0.6; cursor: progress; }
@keyframes resetPulse {
  0%, 100% { box-shadow: 0 0 18px -4px var(--miss-glow); }
  50% { box-shadow: 0 0 26px 0 var(--miss-glow); }
}

/* ---- Arena ---- */
.arena {
  display: grid;
  grid-template-columns: 1fr minmax(190px, 240px) 1fr;
  gap: 1.1rem;
  flex-grow: 1;
  min-height: 0;
}

.side {
  display: flex;
  flex-direction: column;
  padding: 1.2rem;
  gap: 0.9rem;
  min-height: 0;
  transition: box-shadow 0.3s, transform 0.3s;
}
.side-a { --accent-color: var(--player-a); --accent-rgb: var(--player-a-rgb); }
.side-b { --accent-color: var(--player-b); --accent-rgb: var(--player-b-rgb); }
.side.lead {
  box-shadow: 0 0 0 1px color-mix(in srgb, var(--accent-color) 40%, transparent),
              0 24px 60px -34px var(--accent-color);
}
.side.flash {
  animation: sideFlash 1.4s ease;
}
@keyframes sideFlash {
  0% { box-shadow: 0 0 0 2px var(--accent-color), 0 0 60px -10px var(--accent-color); transform: scale(1.012); }
  100% { box-shadow: 0 0 0 1px transparent; transform: scale(1); }
}

.side-head {
  display: flex;
  align-items: center;
  gap: 0.6rem;
}
.side-dot {
  width: 10px; height: 10px; border-radius: 50%;
  background: var(--accent-color);
  box-shadow: 0 0 12px var(--accent-color);
}
.side-name {
  font-size: 1.05rem;
  font-weight: 700;
  letter-spacing: 0.04em;
  color: var(--text-main);
  flex: 1;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.side-b .side-head { flex-direction: row-reverse; text-align: right; }
.new-ping {
  font-size: 0.6rem;
  letter-spacing: 0.16em;
  color: var(--accent-color);
  padding: 3px 8px;
  border-radius: 5px;
  background: color-mix(in srgb, var(--accent-color) 16%, transparent);
  border: 1px solid color-mix(in srgb, var(--accent-color) 40%, transparent);
}

.side-stats {
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  gap: 0.8rem;
}
.side-b .side-stats { flex-direction: row-reverse; }

.side-hero {
  display: flex;
  flex-direction: column;
  gap: 0.1rem;
}
.side-b .side-hero { align-items: flex-end; }
.side-hero-val {
  font-size: 2.4rem;
  font-weight: 800;
  line-height: 1;
  color: var(--accent-color);
  text-shadow: 0 0 22px color-mix(in srgb, var(--accent-color) 60%, transparent);
}
.side-hero-val .u { font-size: 1rem; color: var(--text-muted); }

/* Prominent successful / unsuccessful throw counts */
.side-tally {
  display: flex;
  gap: 0.9rem;
}
.tally {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.1rem;
  min-width: 48px;
  padding: 0.4rem 0.7rem;
  border-radius: 10px;
  background: rgba(0, 0, 0, 0.3);
  border: 1px solid var(--border-color);
}
.tally-num {
  font-size: 1.85rem;
  font-weight: 800;
  line-height: 1;
}
.tally-lbl {
  font-size: 0.55rem;
  letter-spacing: 0.16em;
  color: var(--text-dim);
}
.tally-hit {
  border-color: rgba(47, 245, 184, 0.3);
  background: rgba(47, 245, 184, 0.06);
}
.tally-hit .tally-num { color: var(--hit); text-shadow: 0 0 16px var(--hit-glow); }
.tally-hit .tally-lbl { color: var(--hit); opacity: 0.8; }
.tally-miss {
  border-color: rgba(255, 59, 107, 0.3);
  background: rgba(255, 59, 107, 0.06);
}
.tally-miss .tally-num { color: var(--miss); text-shadow: 0 0 16px var(--miss-glow); }
.tally-miss .tally-lbl { color: var(--miss); opacity: 0.8; }

.side-viewer { flex-grow: 1; min-height: 160px; }

/* ---- Versus center column ---- */
.versus {
  display: flex;
  flex-direction: column;
  justify-content: center;
  gap: 0.9rem;
  padding: 0.5rem 0;
}
.scoreline {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 0.8rem;
  margin-top: 0.8rem;
}
.score-a, .score-b {
  font-size: 2rem;
  font-weight: 800;
  color: var(--text-dim);
  transition: color 0.3s, text-shadow 0.3s;
}
.score-a.win { color: var(--player-a); text-shadow: 0 0 18px rgba(var(--player-a-rgb), 0.6); }
.score-b.win { color: var(--player-b); text-shadow: 0 0 18px rgba(var(--player-b-rgb), 0.6); }
.score-sep { color: var(--text-dim); font-size: 1.5rem; }
.score-label { text-align: center; }

.ping-enter-active, .ping-leave-active { transition: opacity 0.25s, transform 0.25s; }
.ping-enter-from, .ping-leave-to { opacity: 0; transform: translateY(-4px); }

@media (max-width: 1100px) {
  .arena { grid-template-columns: 1fr; overflow-y: auto; }
  .versus { order: -1; }
}
</style>
