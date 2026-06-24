<template>
  <div class="settings-scroll">
    <div class="settings-wrapper glass-panel hud-corners">
      <div class="settings-head">
        <span class="hud-label">System Configuration</span>
        <h3 class="display">Settings</h3>
      </div>

      <div class="settings-section">
        <h4>Accent Theme</h4>
        <p class="desc">Färbt das gesamte HUD um — Glow, Borders und Trajektorien.</p>

        <div class="theme-grid">
          <button
            v-for="t in themes"
            :key="t.name"
            class="theme-card"
            :class="{ active: currentAccent === t.name }"
            :style="{ '--swatch': t.color, '--swatch-2': t.color2 }"
            @click="applyTheme(t)"
          >
            <span class="swatch"></span>
            <span class="theme-name mono">{{ t.label }}</span>
          </button>
        </div>
      </div>

      <div class="settings-section">
        <h4>Connection</h4>
        <p class="desc">MQTT Broker für Echtzeit-Würfe (read-only).</p>
        <div class="input-group">
          <div class="conn-field mono">
            <span class="conn-dot"></span>{{ brokerUrl }}
          </div>
          <button class="btn-secondary" disabled>LOCKED</button>
        </div>
      </div>

      <div class="settings-section danger-zone">
        <h4 class="danger-title">Data Management</h4>
        <p class="desc">Lokale Session-Statistiken zurücksetzen.</p>
        <button class="btn-danger" @click="handleReset">Reset Profile Data</button>
        <transition name="fade">
          <p v-if="resetMessage" class="feedback-msg mono">{{ resetMessage }}</p>
        </transition>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue';

const themes = [
  { name: 'cyan', label: 'CYAN', color: '#22d3ee', color2: '#ff36d7', rgb: '34, 211, 238' },
  { name: 'magenta', label: 'MAGENTA', color: '#ff36d7', color2: '#22d3ee', rgb: '255, 54, 215' },
  { name: 'violet', label: 'VIOLET', color: '#9a6bff', color2: '#22d3ee', rgb: '154, 107, 255' },
  { name: 'emerald', label: 'EMERALD', color: '#2ff5b8', color2: '#22d3ee', rgb: '47, 245, 184' },
  { name: 'amber', label: 'AMBER', color: '#ffb627', color2: '#ff36d7', rgb: '255, 182, 39' },
];

const currentAccent = ref('cyan');
const brokerUrl = ref('wss://test.mosquitto.org:8081');
const resetMessage = ref('');

const applyTheme = (t) => {
  currentAccent.value = t.name;
  const root = document.documentElement.style;
  root.setProperty('--accent-color', t.color);
  root.setProperty('--accent-2', t.color2);
  root.setProperty('--accent-rgb', t.rgb);
  root.setProperty('--accent-glow', `rgba(${t.rgb}, 0.45)`);
};

const handleReset = () => {
  resetMessage.value = '✓ Profildaten für die aktuelle Session geleert.';
  setTimeout(() => { resetMessage.value = ''; }, 3000);
};
</script>

<style scoped>
.settings-scroll { overflow-y: auto; height: 100%; }
.settings-wrapper {
  padding: 1.8rem;
  display: flex;
  flex-direction: column;
  max-width: 720px;
}

.settings-head {
  display: flex;
  flex-direction: column;
  gap: 0.3rem;
  margin-bottom: 2rem;
  padding-bottom: 1.2rem;
  border-bottom: 1px solid var(--border-color);
}
.settings-head h3 {
  font-size: 1.6rem;
  font-weight: 800;
  letter-spacing: 0.04em;
}

.settings-section { margin-bottom: 2rem; }
h4 {
  font-size: 0.95rem;
  margin-bottom: 0.25rem;
  letter-spacing: 0.02em;
}
.desc {
  color: var(--text-muted);
  font-size: 0.82rem;
  margin-bottom: 1rem;
}

/* ---- Theme grid ---- */
.theme-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(110px, 1fr));
  gap: 0.8rem;
}
.theme-card {
  --swatch: var(--accent-color);
  --swatch-2: var(--accent-2);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.6rem;
  padding: 1rem 0.5rem;
  border-radius: 12px;
  background: rgba(0, 0, 0, 0.25);
  border: 1px solid var(--border-color);
  cursor: pointer;
  transition: transform 0.2s, border-color 0.2s, box-shadow 0.2s;
}
.theme-card:hover { transform: translateY(-3px); border-color: var(--swatch); }
.theme-card.active {
  border-color: var(--swatch);
  box-shadow: 0 0 0 1px var(--swatch), 0 10px 30px -16px var(--swatch);
}
.swatch {
  width: 38px;
  height: 38px;
  border-radius: 50%;
  background: linear-gradient(135deg, var(--swatch), var(--swatch-2));
  box-shadow: 0 0 16px -2px var(--swatch);
}
.theme-name { font-size: 0.66rem; color: var(--text-muted); letter-spacing: 0.12em; }
.theme-card.active .theme-name { color: var(--text-main); }

/* ---- Connection ---- */
.input-group { display: flex; gap: 0.8rem; }
.conn-field {
  flex-grow: 1;
  display: flex;
  align-items: center;
  gap: 0.6rem;
  padding: 0.75rem 1rem;
  border-radius: 9px;
  background: var(--bg-inset);
  border: 1px solid var(--border-color);
  color: var(--text-muted);
  font-size: 0.78rem;
}
.conn-dot {
  width: 8px; height: 8px; border-radius: 50%;
  background: var(--hit);
  box-shadow: 0 0 10px var(--hit-glow);
}
.btn-secondary {
  padding: 0.75rem 1.4rem;
  border-radius: 9px;
  background: rgba(255, 255, 255, 0.04);
  border: 1px solid var(--border-color);
  color: var(--text-dim);
  cursor: not-allowed;
  font-family: var(--font-mono);
  font-size: 0.7rem;
  letter-spacing: 0.12em;
}

/* ---- Danger ---- */
.danger-zone {
  border: 1px solid rgba(255, 59, 107, 0.25);
  background: rgba(255, 59, 107, 0.05);
  padding: 1.4rem;
  border-radius: 12px;
  margin-bottom: 0;
}
.danger-title { color: var(--miss); }
.btn-danger {
  padding: 0.75rem 1.5rem;
  border-radius: 9px;
  background: linear-gradient(135deg, var(--miss), #c81e4a);
  border: none;
  color: white;
  font-weight: 600;
  cursor: pointer;
  transition: transform 0.15s, box-shadow 0.2s;
}
.btn-danger:hover {
  transform: translateY(-2px);
  box-shadow: 0 10px 26px -12px var(--miss-glow);
}
.feedback-msg {
  margin-top: 1rem;
  color: var(--hit);
  font-size: 0.78rem;
}

.fade-enter-active, .fade-leave-active { transition: opacity 0.3s; }
.fade-enter-from, .fade-leave-to { opacity: 0; }
</style>
