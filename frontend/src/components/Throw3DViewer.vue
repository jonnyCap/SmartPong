<template>
  <div class="viewer-wrapper glass-panel hud-corners" :class="{ compact }">
    <div class="viewer-head" v-if="!compact">
      <span class="hud-label">Trajectory · 3D</span>
      <span class="viewer-legend mono">
        <span class="lg lg-hit">HIT</span>
        <span class="lg lg-miss">MISS</span>
      </span>
    </div>
    <div class="canvas-container" ref="canvasContainer"></div>
    <div v-if="!currentPathData || currentPathData.length === 0" class="overlay">
      <div class="overlay-inner">
        <div class="radar"></div>
        <p class="mono">Warte auf Wurfdaten…</p>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, watch, computed } from 'vue';
import * as THREE from 'three';

const props = defineProps({
  latestTrajectory: { type: Object, default: null },
  userProfile: { type: Object, default: null },
  viewMode: { type: String, default: 'all' },
  accentColor: { type: String, default: '#22d3ee' }, // grid / ball / lights
  forceColor: { type: String, default: '' },          // override trajectory color (duel)
  compact: { type: Boolean, default: false },
});

const canvasContainer = ref(null);
let scene, camera, renderer, animationId;
let throwTube, throwGlow, ballMesh, ballLight, gridHelper;
let moveBallAnimationId;

const currentPathData = computed(() => {
  if (props.viewMode === 'latest' && props.latestTrajectory && props.latestTrajectory.path) {
    return props.latestTrajectory.path;
  }
  if (!props.userProfile || !props.userProfile.throws || props.userProfile.throws.length === 0) {
    return [];
  }
  if (props.viewMode.startsWith('history-')) {
    const idx = parseInt(props.viewMode.split('-')[1]);
    return props.userProfile.throws[idx]?.trajectory_path || [];
  }
  if (props.viewMode.startsWith('avg-') || props.viewMode === 'all' || props.viewMode === 'hits' || props.viewMode === 'misses') {
    let throwsToAvg = props.userProfile.throws;
    if (props.viewMode === 'avg-hits' || props.viewMode === 'hits') {
      throwsToAvg = throwsToAvg.filter((t) => t.is_hit);
    } else if (props.viewMode === 'avg-misses' || props.viewMode === 'misses') {
      throwsToAvg = throwsToAvg.filter((t) => !t.is_hit);
    }
    if (throwsToAvg.length === 0) return [];

    let minLen = Math.min(...throwsToAvg.map((t) => (t.trajectory_path || []).length));
    if (minLen === Infinity || minLen === 0) return [];

    const avgPath = [];
    for (let i = 0; i < minLen; i++) {
      let sumX = 0, sumY = 0, sumZ = 0;
      for (const t of throwsToAvg) {
        sumX += t.trajectory_path[i][0];
        sumY += t.trajectory_path[i][1];
        sumZ += t.trajectory_path[i][2];
      }
      avgPath.push([sumX / throwsToAvg.length, sumY / throwsToAvg.length, sumZ / throwsToAvg.length]);
    }
    return avgPath;
  }
  return [];
});

const trajectoryColor = computed(() => {
  if (props.forceColor) return props.forceColor;
  // outcome-based fallback inferred from the view mode
  if (props.viewMode.includes('miss')) return '#ff3b6b';
  return '#2ff5b8';
});

const initThreeJS = () => {
  const width = canvasContainer.value.clientWidth;
  const height = canvasContainer.value.clientHeight;

  scene = new THREE.Scene();
  scene.fog = new THREE.FogExp2(0x04060e, 0.035);

  camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 100);
  camera.position.set(5, 5, 10);
  camera.lookAt(0, 0, 0);

  renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setSize(width, height);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  canvasContainer.value.appendChild(renderer.domElement);

  const accent = new THREE.Color(props.accentColor);

  scene.add(new THREE.AmbientLight(0xffffff, 0.35));

  const keyLight = new THREE.PointLight(accent.getHex(), 1.4, 60);
  keyLight.position.set(6, 7, 6);
  scene.add(keyLight);

  const rimLight = new THREE.PointLight(0xff36d7, 0.8, 60);
  rimLight.position.set(-7, 3, -5);
  scene.add(rimLight);

  // Neon grid
  gridHelper = new THREE.GridHelper(12, 12, accent.getHex(), 0x1b2740);
  gridHelper.material.opacity = 0.45;
  gridHelper.material.transparent = true;
  gridHelper.position.y = -2;
  scene.add(gridHelper);

  // Glowing ball (the watch)
  const geometry = new THREE.SphereGeometry(0.22, 32, 32);
  const material = new THREE.MeshStandardMaterial({
    color: accent.getHex(),
    emissive: accent.getHex(),
    emissiveIntensity: 1.4,
    roughness: 0.25,
    metalness: 0.3,
  });
  ballMesh = new THREE.Mesh(geometry, material);
  scene.add(ballMesh);

  ballLight = new THREE.PointLight(accent.getHex(), 1.2, 12);
  ballMesh.add(ballLight);

  const clock = { t: 0 };
  const animate = () => {
    animationId = requestAnimationFrame(animate);
    clock.t += 0.0045;
    if (camera) {
      camera.position.x = Math.sin(clock.t) * 11;
      camera.position.z = Math.cos(clock.t) * 11;
      camera.position.y = 5 + Math.sin(clock.t * 0.6) * 1.5;
      camera.lookAt(0, 0, 0);
    }
    renderer.render(scene, camera);
  };
  animate();

  window.addEventListener('resize', onWindowResize);
};

const onWindowResize = () => {
  if (!canvasContainer.value || !camera || !renderer) return;
  const width = canvasContainer.value.clientWidth;
  const height = canvasContainer.value.clientHeight;
  camera.aspect = width / height;
  camera.updateProjectionMatrix();
  renderer.setSize(width, height);
};

const disposeMesh = (mesh) => {
  if (!mesh) return;
  scene.remove(mesh);
  if (mesh.geometry) mesh.geometry.dispose();
  if (mesh.material) mesh.material.dispose();
};

const updateTrajectory = (pathData) => {
  disposeMesh(throwTube);
  disposeMesh(throwGlow);
  throwTube = throwGlow = null;
  if (moveBallAnimationId) cancelAnimationFrame(moveBallAnimationId);

  if (!pathData || pathData.length < 2) return;

  const points = pathData.map((p) => new THREE.Vector3(p[0] * 0.1, p[1] * 0.1, p[2] * 0.1));
  const curve = new THREE.CatmullRomCurve3(points);
  const color = new THREE.Color(trajectoryColor.value);

  // Core neon tube
  const tubeGeo = new THREE.TubeGeometry(curve, 80, 0.045, 10, false);
  const tubeMat = new THREE.MeshBasicMaterial({ color, transparent: true, opacity: 0.95 });
  throwTube = new THREE.Mesh(tubeGeo, tubeMat);
  scene.add(throwTube);

  // Wider additive glow tube
  const glowGeo = new THREE.TubeGeometry(curve, 80, 0.13, 10, false);
  const glowMat = new THREE.MeshBasicMaterial({
    color,
    transparent: true,
    opacity: 0.22,
    blending: THREE.AdditiveBlending,
    depthWrite: false,
  });
  throwGlow = new THREE.Mesh(glowGeo, glowMat);
  scene.add(throwGlow);

  // Tint the ball + its light to the trajectory color
  if (ballMesh) {
    ballMesh.material.color.copy(color);
    ballMesh.material.emissive.copy(color);
    if (ballLight) ballLight.color.copy(color);
  }

  let progress = 0;
  const moveBall = () => {
    progress += 0.012;
    if (progress <= 1) {
      ballMesh.position.copy(curve.getPointAt(progress));
      moveBallAnimationId = requestAnimationFrame(moveBall);
    } else {
      progress = 0;
      moveBallAnimationId = requestAnimationFrame(moveBall);
    }
  };
  moveBall();
};

watch(currentPathData, (newPath) => updateTrajectory(newPath), { deep: true });
watch(() => props.forceColor, () => updateTrajectory(currentPathData.value));

onMounted(() => {
  initThreeJS();
  if (currentPathData.value) updateTrajectory(currentPathData.value);
});

onUnmounted(() => {
  window.removeEventListener('resize', onWindowResize);
  if (animationId) cancelAnimationFrame(animationId);
  if (moveBallAnimationId) cancelAnimationFrame(moveBallAnimationId);
  if (renderer) {
    renderer.dispose();
    if (renderer.domElement) renderer.domElement.remove();
  }
});
</script>

<style scoped>
.viewer-wrapper {
  padding: 1.4rem;
  display: flex;
  flex-direction: column;
  position: relative;
  overflow: hidden;
  gap: 0.9rem;
}
.viewer-wrapper.compact { padding: 0; gap: 0; border-radius: 12px; }

.viewer-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.viewer-legend { display: flex; gap: 0.8rem; font-size: 0.6rem; }
.lg { display: flex; align-items: center; gap: 0.3rem; letter-spacing: 0.1em; }
.lg::before {
  content: "";
  width: 14px; height: 3px; border-radius: 2px;
}
.lg-hit { color: var(--hit); }
.lg-hit::before { background: var(--hit); box-shadow: 0 0 8px var(--hit-glow); }
.lg-miss { color: var(--miss); }
.lg-miss::before { background: var(--miss); box-shadow: 0 0 8px var(--miss-glow); }

.canvas-container {
  flex-grow: 1;
  width: 100%;
  border-radius: 12px;
  overflow: hidden;
  background:
    radial-gradient(80% 80% at 50% 30%, rgba(var(--accent-rgb), 0.08), transparent 70%),
    #03050c;
  min-height: 200px;
}
.compact .canvas-container { border-radius: 12px; }

.overlay {
  position: absolute;
  inset: 1.4rem;
  top: auto;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(4, 6, 14, 0.55);
  backdrop-filter: blur(3px);
  border-radius: 12px;
}
.compact .overlay { inset: 0; }
.overlay-inner { display: flex; flex-direction: column; align-items: center; gap: 1rem; }
.overlay p {
  color: var(--text-muted);
  font-size: 0.72rem;
  letter-spacing: 0.16em;
  animation: floatPulse 1.6s infinite;
}
.radar {
  width: 46px;
  height: 46px;
  border-radius: 50%;
  border: 2px solid color-mix(in srgb, var(--accent-color) 35%, transparent);
  position: relative;
}
.radar::before {
  content: "";
  position: absolute;
  inset: -2px;
  border-radius: 50%;
  border: 2px solid transparent;
  border-top-color: var(--accent-color);
  animation: radarSpin 1.1s linear infinite;
}
@keyframes radarSpin { to { transform: rotate(360deg); } }
</style>
