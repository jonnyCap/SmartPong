<template>
  <div class="app-container">
    <aside class="sidebar glass-panel hud-corners">
      <div class="logo">
        <div class="logo-mark">
          <span class="logo-ping"></span>
        </div>
        <div class="logo-text">
          <h1 class="text-gradient display">SMARTPONG</h1>
          <span class="hud-label">NEON · HUD</span>
        </div>
      </div>

      <nav>
        <ul>
          <li
            v-for="item in navItems"
            :key="item.id"
            :class="{ active: currentView === item.id }"
            @click="currentView = item.id"
          >
            <span class="nav-bar"></span>
            <span class="icon">{{ item.icon }}</span>
            <span class="nav-label">{{ item.label }}</span>
          </li>
        </ul>
      </nav>

      <div class="user-selector">
        <label class="hud-label" for="user">Current Player</label>
        <PlayerSelect
          v-model="selectedUser"
          :users="availableUsers"
          direction="up"
          placeholder="Warte auf Daten…"
        />
      </div>
    </aside>

    <main class="main-content">
      <header class="top-header glass-panel">
        <div class="header-left">
          <div class="header-title">
            <h2 class="display">{{ headerTitle }}</h2>
            <span class="badge" :class="{ live: mqttConnected }">{{ mqttConnected ? 'LIVE' : 'OFF' }}</span>
          </div>

          <div v-if="currentView === 'dashboard'" class="view-select-wrap">
            <span class="hud-label">View</span>
            <select v-model="viewMode" class="view-selector">
              <option value="all">All Throws (Average)</option>
              <option value="hits">Hits Only (Average)</option>
              <option value="misses">Misses Only (Average)</option>
              <option value="latest">Latest Throw</option>
              <optgroup label="Historical Throws">
                <option
                  v-for="(throwData, index) in reversedThrows"
                  :key="index"
                  :value="'history-' + (userProfile?.throws?.length - 1 - index)"
                >
                  Throw #{{ userProfile?.throws?.length - index }} ({{ throwData.is_hit ? 'Hit' : 'Miss' }})
                </option>
              </optgroup>
            </select>
          </div>
        </div>

        <div class="status-indicator" :class="{ connected: mqttConnected }">
          <span class="dot"></span>
          <span class="mono">{{ mqttConnected ? 'BROKER ONLINE' : 'CONNECTING…' }}</span>
        </div>
      </header>

      <div v-if="currentView === 'dashboard'" class="dashboard-grid">
        <Dashboard
          :user-profile="userProfile"
          :latest-throw="latestThrow"
          :view-mode="viewMode"
          class="dashboard-stats"
        />
        <Throw3DViewer
          :latest-trajectory="latestThrow?.trajectory"
          :user-profile="userProfile"
          :view-mode="viewMode"
          class="dashboard-3d"
        />
      </div>

      <DuelMode v-else-if="currentView === 'duel'" :users="availableUsers" />

      <Settings v-else-if="currentView === 'settings'" />
    </main>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, computed, watch } from 'vue';
import Dashboard from './components/Dashboard.vue';
import Throw3DViewer from './components/Throw3DViewer.vue';
import Settings from './components/Settings.vue';
import DuelMode from './components/DuelMode.vue';
import PlayerSelect from './components/PlayerSelect.vue';
import { fetchUserProfile, fetchUsers } from './services/api';
import { connectMqtt, onResult, onStatus, disconnectMqtt } from './services/mqttClient';

const navItems = [
  { id: 'dashboard', label: 'Dashboard', icon: '◈' },
  { id: 'duel', label: 'Duel Mode', icon: '⚔' },
  { id: 'settings', label: 'Settings', icon: '⚙' },
];

const selectedUser = ref('player1');
const currentView = ref('dashboard');
const userProfile = ref(null);
const latestThrow = ref(null);
const mqttConnected = ref(false);
const availableUsers = ref([]); // array of summary objects
const viewMode = ref('all');

let unsubResult = null;
let unsubStatus = null;

const headerTitle = computed(() => {
  if (currentView.value === 'duel') return 'Duel Mode';
  if (currentView.value === 'settings') return 'Settings';
  return 'Overview';
});

const reversedThrows = computed(() => {
  if (!userProfile.value || !userProfile.value.throws) return [];
  return [...userProfile.value.throws].reverse();
});

const loadUsers = async () => {
  const users = await fetchUsers();
  if (users && users.length > 0) {
    availableUsers.value = users;
    const ids = users.map((u) => u.user_id);
    if (!ids.includes(selectedUser.value)) {
      selectedUser.value = ids[0];
    }
  }
};

const loadProfile = async () => {
  if (!selectedUser.value) return;
  userProfile.value = await fetchUserProfile(selectedUser.value);
};

// React to player changes from the custom dropdown.
watch(selectedUser, () => {
  latestThrow.value = null;
  loadProfile();
});

onMounted(() => {
  loadUsers();
  loadProfile();

  // Refresh users list periodically (keeps "last seen" fresh).
  setInterval(loadUsers, 5000);

  connectMqtt();
  unsubStatus = onStatus((connected) => { mqttConnected.value = connected; });
  unsubResult = onResult((payload) => {
    if (payload.user_id === selectedUser.value) {
      latestThrow.value = payload;
      loadProfile();
    }
    // A throw from any player updates the picker's "last seen".
    loadUsers();
  });
});

onUnmounted(() => {
  if (unsubResult) unsubResult();
  if (unsubStatus) unsubStatus();
  disconnectMqtt();
});
</script>

<style scoped>
.app-container {
  display: flex;
  height: 100vh;
  padding: 1.25rem;
  gap: 1.25rem;
}

/* ---------- Sidebar ---------- */
.sidebar {
  width: 270px;
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  padding: 1.5rem 1.25rem;
}

.logo {
  display: flex;
  align-items: center;
  gap: 0.85rem;
  margin-bottom: 2.25rem;
  padding-bottom: 1.25rem;
  border-bottom: 1px solid var(--border-color);
}
.logo-mark {
  width: 42px;
  height: 42px;
  border-radius: 12px;
  flex-shrink: 0;
  display: grid;
  place-items: center;
  background: radial-gradient(circle at 30% 30%, rgba(var(--accent-rgb), 0.35), rgba(255, 54, 215, 0.18));
  border: 1px solid color-mix(in srgb, var(--accent-color) 50%, transparent);
  box-shadow: inset 0 0 18px -6px var(--accent-color), 0 0 22px -10px var(--accent-color);
}
.logo-ping {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  background: var(--accent-color);
  box-shadow: 0 0 12px var(--accent-color);
  animation: floatPulse 1.8s infinite;
}
.logo-text h1 {
  font-size: 1.3rem;
  line-height: 1;
  font-weight: 800;
  letter-spacing: 0.04em;
}
.logo-text .hud-label {
  font-size: 0.6rem;
  letter-spacing: 0.32em;
}

nav ul {
  list-style: none;
  flex-grow: 1;
}
nav li {
  position: relative;
  padding: 0.85rem 1rem;
  margin-bottom: 0.4rem;
  border-radius: 10px;
  cursor: pointer;
  display: flex;
  align-items: center;
  gap: 0.85rem;
  color: var(--text-muted);
  transition: all 0.2s ease;
  overflow: hidden;
}
.nav-bar {
  position: absolute;
  left: 0;
  top: 50%;
  transform: translateY(-50%) scaleY(0);
  width: 3px;
  height: 60%;
  border-radius: 2px;
  background: linear-gradient(var(--accent-color), var(--accent-2));
  box-shadow: 0 0 12px var(--accent-color);
  transition: transform 0.2s ease;
}
nav li .icon {
  font-size: 1.05rem;
  width: 1.4rem;
  text-align: center;
}
.nav-label { font-weight: 500; letter-spacing: 0.02em; }
nav li:hover {
  background: rgba(125, 170, 255, 0.06);
  color: var(--text-main);
}
nav li.active {
  background: linear-gradient(100deg, rgba(var(--accent-rgb), 0.16), rgba(255, 54, 215, 0.06));
  color: var(--text-main);
}
nav li.active .nav-bar { transform: translateY(-50%) scaleY(1); }
nav li.active .icon {
  color: var(--accent-color);
  text-shadow: 0 0 12px var(--accent-color);
}

.user-selector {
  margin-top: auto;
  padding-top: 1.25rem;
  border-top: 1px solid var(--border-color);
  display: flex;
  flex-direction: column;
  gap: 0.55rem;
}

/* ---------- Main ---------- */
.main-content {
  flex-grow: 1;
  display: flex;
  flex-direction: column;
  gap: 1.25rem;
  min-width: 0;
}

.top-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0.85rem 1.25rem;
}
.header-left {
  display: flex;
  align-items: center;
  gap: 1.5rem;
}
.header-title {
  display: flex;
  align-items: center;
  gap: 0.7rem;
}
.header-title h2 {
  font-size: 1.15rem;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
}

.badge {
  font-family: var(--font-mono);
  font-size: 0.6rem;
  letter-spacing: 0.15em;
  background: rgba(255, 59, 107, 0.18);
  color: var(--miss);
  padding: 3px 8px;
  border-radius: 5px;
  border: 1px solid rgba(255, 59, 107, 0.4);
}
.badge.live {
  background: rgba(47, 245, 184, 0.14);
  color: var(--hit);
  border-color: rgba(47, 245, 184, 0.4);
  box-shadow: 0 0 14px -4px var(--hit-glow);
}

.view-select-wrap {
  display: flex;
  align-items: center;
  gap: 0.6rem;
  padding-left: 1.5rem;
  border-left: 1px solid var(--border-color);
}
.view-selector {
  padding: 0.5rem 0.8rem;
  border-radius: 9px;
  background: var(--bg-inset);
  border: 1px solid var(--border-color);
  color: var(--text-main);
  outline: none;
  font-size: 0.82rem;
  font-family: inherit;
  cursor: pointer;
  transition: border-color 0.2s;
}
.view-selector:hover, .view-selector:focus {
  border-color: var(--border-bright);
}
.view-selector option, .view-selector optgroup {
  background: var(--bg-panel-solid);
  color: var(--text-main);
}

.status-indicator {
  display: flex;
  align-items: center;
  gap: 0.55rem;
  font-size: 0.68rem;
  letter-spacing: 0.12em;
  color: var(--text-muted);
}
.status-indicator .dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--miss);
  box-shadow: 0 0 8px var(--miss-glow);
}
.status-indicator.connected .dot {
  background: var(--hit);
  box-shadow: 0 0 10px var(--hit-glow);
  animation: ringPulse 2s infinite;
}

.dashboard-grid {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(0, 1.05fr);
  gap: 1.25rem;
  flex-grow: 1;
  min-height: 0;
}
.dashboard-stats, .dashboard-3d {
  display: flex;
  flex-direction: column;
  min-height: 0;
}

@media (max-width: 1100px) {
  .dashboard-grid { grid-template-columns: 1fr; overflow-y: auto; }
}
</style>
