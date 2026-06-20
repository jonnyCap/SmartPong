<template>
  <div class="settings-wrapper glass-panel">
    <h3>Settings</h3>

    <div class="settings-section">
      <h4>Theme Appearance</h4>
      <p class="desc">Personalize your dashboard accent color.</p>
      
      <div class="color-options">
        <button 
          class="color-btn blue" 
          :class="{ active: currentAccent === 'blue' }"
          @click="setAccentColor('blue', '#3B82F6', 'rgba(59, 130, 246, 0.5)')"
        ></button>
        <button 
          class="color-btn purple" 
          :class="{ active: currentAccent === 'purple' }"
          @click="setAccentColor('purple', '#8B5CF6', 'rgba(139, 92, 246, 0.5)')"
        ></button>
        <button 
          class="color-btn emerald" 
          :class="{ active: currentAccent === 'emerald' }"
          @click="setAccentColor('emerald', '#10B981', 'rgba(16, 185, 129, 0.5)')"
        ></button>
      </div>
    </div>

    <div class="settings-section mt-4">
      <h4>Connection Configuration</h4>
      <p class="desc">MQTT Broker URL used for real-time 3D throws.</p>
      
      <div class="input-group">
        <input type="text" v-model="brokerUrl" disabled />
        <button class="btn-secondary" disabled>Edit (Locked)</button>
      </div>
    </div>

    <div class="settings-section mt-4 danger-zone">
      <h4>Data Management</h4>
      <p class="desc">Clear your local profile session stats.</p>
      
      <button class="btn-danger" @click="handleReset">Reset Profile Data</button>
      <p v-if="resetMessage" class="feedback-msg">{{ resetMessage }}</p>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue';

const currentAccent = ref('blue');
const brokerUrl = ref('wss://test.mosquitto.org:8081');
const resetMessage = ref('');

const setAccentColor = (name, hex, glow) => {
  currentAccent.value = name;
  document.documentElement.style.setProperty('--accent-color', hex);
  document.documentElement.style.setProperty('--accent-glow', glow);
};

const handleReset = () => {
  // In a real app, this would call an API or clear localStorage
  resetMessage.value = "Profile data cleared for the current session!";
  setTimeout(() => {
    resetMessage.value = '';
  }, 3000);
};
</script>

<style scoped>
.settings-wrapper {
  padding: 1.5rem;
  display: flex;
  flex-direction: column;
}

h3 {
  color: var(--text-main);
  font-size: 1.25rem;
  margin-bottom: 2rem;
  border-bottom: 1px solid var(--border-color);
  padding-bottom: 1rem;
}

.settings-section {
  margin-bottom: 2rem;
}

.mt-4 {
  margin-top: 1rem;
}

h4 {
  color: var(--text-main);
  font-size: 1rem;
  margin-bottom: 0.25rem;
}

.desc {
  color: var(--text-muted);
  font-size: 0.85rem;
  margin-bottom: 1rem;
}

.color-options {
  display: flex;
  gap: 1rem;
}

.color-btn {
  width: 40px;
  height: 40px;
  border-radius: 50%;
  border: 2px solid transparent;
  cursor: pointer;
  transition: transform 0.2s;
}

.color-btn:hover {
  transform: scale(1.1);
}

.color-btn.active {
  border-color: white;
  box-shadow: 0 0 10px rgba(255, 255, 255, 0.3);
}

.color-btn.blue { background-color: #3B82F6; }
.color-btn.purple { background-color: #8B5CF6; }
.color-btn.emerald { background-color: #10B981; }

.input-group {
  display: flex;
  gap: 1rem;
}

input[type="text"] {
  flex-grow: 1;
  padding: 0.75rem 1rem;
  border-radius: 8px;
  background: rgba(0, 0, 0, 0.2);
  border: 1px solid var(--border-color);
  color: var(--text-muted);
  font-family: inherit;
}

.btn-secondary {
  padding: 0.75rem 1.5rem;
  border-radius: 8px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid var(--border-color);
  color: var(--text-muted);
  cursor: not-allowed;
}

.danger-zone {
  border: 1px solid rgba(239, 68, 68, 0.2);
  background: rgba(239, 68, 68, 0.05);
  padding: 1.5rem;
  border-radius: 12px;
}

.btn-danger {
  padding: 0.75rem 1.5rem;
  border-radius: 8px;
  background: var(--danger-color);
  border: none;
  color: white;
  font-weight: 600;
  cursor: pointer;
  transition: opacity 0.2s;
}

.btn-danger:hover {
  opacity: 0.9;
}

.feedback-msg {
  margin-top: 1rem;
  color: var(--success-color);
  font-size: 0.9rem;
  font-weight: 500;
}
</style>
