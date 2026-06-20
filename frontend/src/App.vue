<template>
  <div class="app-container">
    <aside class="sidebar glass-panel">
      <div class="logo">
        <h1 class="text-gradient">SmartPong</h1>
      </div>
      <nav>
        <ul>
          <li :class="{ active: currentView === 'dashboard' }" @click="currentView = 'dashboard'">
            <span class="icon">📊</span>
            Dashboard
          </li>
          <li :class="{ active: currentView === 'settings' }" @click="currentView = 'settings'">
            <span class="icon">⚙️</span>
            Settings
          </li>
        </ul>
      </nav>
      <div class="user-selector">
        <label for="user">Current Player</label>
        <select id="user" v-model="selectedUser" @change="handleUserChange">
          <option v-for="u in availableUsers" :key="u" :value="u">{{ u }}</option>
          <option v-if="availableUsers.length === 0" value="player1">Waiting for data...</option>
        </select>
      </div>
    </aside>
    
    <main class="main-content">
      <header class="top-header">
        <div class="header-left">
          <h2>Overview <span class="badge">Live</span></h2>
          <select v-model="viewMode" class="view-selector glass-panel">
            <option value="all">All Throws (Average)</option>
            <option value="hits">Hits Only (Average)</option>
            <option value="misses">Misses Only (Average)</option>
            <option value="latest">Latest Throw</option>
            <optgroup label="Historical Throws">
              <option v-for="(throwData, index) in reversedThrows" :key="index" :value="'history-' + (userProfile?.throws?.length - 1 - index)">
                Throw #{{ userProfile?.throws?.length - index }} ({{ throwData.is_hit ? 'Hit' : 'Miss' }})
              </option>
            </optgroup>
          </select>
        </div>
        <div class="status-indicator" :class="{ 'connected': mqttConnected }">
          <span class="dot"></span>
          {{ mqttConnected ? 'Broker Connected' : 'Connecting...' }}
        </div>
      </header>
      
      <div v-if="currentView === 'dashboard'" class="dashboard-grid">
        <Dashboard :user-profile="userProfile" :latest-throw="latestThrow" :view-mode="viewMode" class="dashboard-stats" />
        <Throw3DViewer :latest-trajectory="latestThrow?.trajectory" :user-profile="userProfile" :view-mode="viewMode" class="dashboard-3d" />
      </div>
      
      <Settings v-else-if="currentView === 'settings'" />
    </main>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, computed } from 'vue';
import Dashboard from './components/Dashboard.vue';
import Throw3DViewer from './components/Throw3DViewer.vue';
import Settings from './components/Settings.vue';
import { fetchUserProfile, fetchUsers } from './services/api';
import { connectMqtt, disconnectMqtt } from './services/mqttClient';

const selectedUser = ref('player1');
const currentView = ref('dashboard');
const userProfile = ref(null);
const latestThrow = ref(null);
const mqttConnected = ref(false);
const availableUsers = ref([]);
const viewMode = ref('all');

const reversedThrows = computed(() => {
  if (!userProfile.value || !userProfile.value.throws) return [];
  return [...userProfile.value.throws].reverse();
});

const loadUsers = async () => {
  const users = await fetchUsers();
  if (users && users.length > 0) {
    availableUsers.value = users;
    if (!availableUsers.value.includes(selectedUser.value)) {
      selectedUser.value = availableUsers.value[0];
      loadProfile();
    }
  }
};

const loadProfile = async () => {
  if (!selectedUser.value) return;
  userProfile.value = await fetchUserProfile(selectedUser.value);
};

const handleUserChange = () => {
  loadProfile();
  latestThrow.value = null; // Clear latest throw for new user
};

onMounted(() => {
  loadUsers();
  loadProfile();
  
  // Refresh users list occasionally
  setInterval(loadUsers, 5000);
  
  connectMqtt((payload) => {
    mqttConnected.value = true;
    if (payload.user_id === selectedUser.value) {
      latestThrow.value = payload;
      // Also reload profile to get updated stats
      loadProfile();
    }
  });
  
  // Fake connection status update for UI if it takes a moment
  setTimeout(() => {
    if (!latestThrow.value) mqttConnected.value = true;
  }, 1000);
});

onUnmounted(() => {
  disconnectMqtt();
});
</script>

<style scoped>
.app-container {
  display: flex;
  height: 100vh;
  padding: 1.5rem;
  gap: 1.5rem;
}

.sidebar {
  width: 260px;
  display: flex;
  flex-direction: column;
  padding: 1.5rem;
}

.logo h1 {
  font-size: 1.5rem;
  margin-bottom: 2rem;
  text-align: center;
}

nav ul {
  list-style: none;
  flex-grow: 1;
}

nav li {
  padding: 1rem;
  margin-bottom: 0.5rem;
  border-radius: 8px;
  cursor: pointer;
  display: flex;
  align-items: center;
  gap: 0.75rem;
  color: var(--text-muted);
  transition: all 0.2s ease;
}

nav li:hover {
  background: rgba(255, 255, 255, 0.05);
  color: var(--text-main);
}

nav li.active {
  background: var(--accent-glow);
  color: var(--text-main);
  border: 1px solid rgba(59, 130, 246, 0.3);
}

.user-selector {
  margin-top: auto;
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.user-selector label {
  font-size: 0.85rem;
  color: var(--text-muted);
}

.user-selector select {
  padding: 0.75rem;
  border-radius: 8px;
  background: rgba(0, 0, 0, 0.2);
  border: 1px solid var(--border-color);
  color: var(--text-main);
  outline: none;
}

.main-content {
  flex-grow: 1;
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
}

.top-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 0.5rem;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 1.5rem;
}

.view-selector {
  padding: 0.5rem;
  border-radius: 8px;
  background: rgba(0, 0, 0, 0.4);
  border: 1px solid var(--border-color);
  color: var(--text-main);
  outline: none;
  font-size: 0.85rem;
}

.badge {
  font-size: 0.75rem;
  background: var(--danger-color);
  color: white;
  padding: 2px 8px;
  border-radius: 12px;
  vertical-align: super;
  margin-left: 0.5rem;
}

.status-indicator {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.85rem;
  color: var(--text-muted);
}

.dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--danger-color);
}

.status-indicator.connected .dot {
  background: var(--success-color);
  box-shadow: 0 0 8px var(--success-color);
}

.dashboard-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1.5rem;
  flex-grow: 1;
  min-height: 0;
}

.dashboard-stats {
  display: flex;
  flex-direction: column;
}

.dashboard-3d {
  display: flex;
  flex-direction: column;
}
</style>
