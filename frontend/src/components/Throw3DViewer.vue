<template>
  <div class="viewer-wrapper glass-panel">
    <div class="canvas-container" ref="canvasContainer"></div>
    <div v-if="!currentPathData || currentPathData.length === 0" class="overlay">
      <p>Waiting for throw data...</p>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, watch, computed } from 'vue';
import * as THREE from 'three';

const props = defineProps({
  latestTrajectory: {
    type: Object,
    default: null
  },
  userProfile: {
    type: Object,
    default: null
  },
  viewMode: {
    type: String,
    default: 'all'
  }
});

const canvasContainer = ref(null);
let scene, camera, renderer, animationId;
let throwPath, ballMesh;

const reversedThrows = computed(() => {
  if (!props.userProfile || !props.userProfile.throws) return [];
  return [...props.userProfile.throws].reverse();
});

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
      throwsToAvg = throwsToAvg.filter(t => t.is_hit);
    } else if (props.viewMode === 'avg-misses' || props.viewMode === 'misses') {
      throwsToAvg = throwsToAvg.filter(t => !t.is_hit);
    }
    
    if (throwsToAvg.length === 0) return [];
    
    // Average trajectories (simple average of points, assuming similar length)
    // Find min length
    let minLen = Math.min(...throwsToAvg.map(t => (t.trajectory_path || []).length));
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

const initThreeJS = () => {
  const width = canvasContainer.value.clientWidth;
  const height = canvasContainer.value.clientHeight;

  // Scene
  scene = new THREE.Scene();
  scene.background = new THREE.Color('#0F172A');
  scene.fog = new THREE.Fog('#0F172A', 10, 50);

  // Camera
  camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 100);
  camera.position.set(5, 5, 10);
  camera.lookAt(0, 0, 0);

  // Renderer
  renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setSize(width, height);
  renderer.setPixelRatio(window.devicePixelRatio);
  canvasContainer.value.appendChild(renderer.domElement);

  // Lighting
  const ambientLight = new THREE.AmbientLight(0xffffff, 0.5);
  scene.add(ambientLight);
  
  const pointLight = new THREE.PointLight(0x3B82F6, 1);
  pointLight.position.set(5, 5, 5);
  scene.add(pointLight);

  // Grid
  const gridHelper = new THREE.GridHelper(10, 10, 0x3B82F6, 0xffffff);
  gridHelper.material.opacity = 0.2;
  gridHelper.material.transparent = true;
  scene.add(gridHelper);

  // Ball (Watch representation)
  const geometry = new THREE.SphereGeometry(0.2, 32, 32);
  const material = new THREE.MeshPhongMaterial({ 
    color: 0x3B82F6,
    emissive: 0x1E3A8A,
    shininess: 100
  });
  ballMesh = new THREE.Mesh(geometry, material);
  scene.add(ballMesh);

  // Animation Loop
  const animate = () => {
    animationId = requestAnimationFrame(animate);
    
    if (camera) {
      camera.position.x = Math.sin(Date.now() * 0.0005) * 10;
      camera.position.z = Math.cos(Date.now() * 0.0005) * 10;
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

let moveBallAnimationId;

const updateTrajectory = (pathData) => {
  if (throwPath) {
    scene.remove(throwPath);
  }
  if (moveBallAnimationId) {
    cancelAnimationFrame(moveBallAnimationId);
  }

  if (!pathData || pathData.length < 2) return;

  const points = pathData.map(p => new THREE.Vector3(p[0] * 0.1, p[1] * 0.1, p[2] * 0.1));
  
  // Create a smooth curve
  const curve = new THREE.CatmullRomCurve3(points);
  const curvePoints = curve.getPoints(50);
  const geometry = new THREE.BufferGeometry().setFromPoints(curvePoints);
  
  const material = new THREE.LineBasicMaterial({ 
    color: props.viewMode.includes('miss') ? 0xEF4444 : 0x10B981,
    linewidth: 2 
  });

  throwPath = new THREE.Line(geometry, material);
  scene.add(throwPath);

  // Animate ball along the curve
  let progress = 0;
  
  const moveBall = () => {
    progress += 0.01;
    if (progress <= 1) {
      const pos = curve.getPointAt(progress);
      ballMesh.position.copy(pos);
      moveBallAnimationId = requestAnimationFrame(moveBall);
    }
  };
  moveBall();
};

watch(currentPathData, (newPath) => {
  updateTrajectory(newPath);
}, { deep: true });

onMounted(() => {
  initThreeJS();
  if (currentPathData.value) {
    updateTrajectory(currentPathData.value);
  }
});

onUnmounted(() => {
  window.removeEventListener('resize', onWindowResize);
  if (animationId) cancelAnimationFrame(animationId);
  if (moveBallAnimationId) cancelAnimationFrame(moveBallAnimationId);
  if (renderer && renderer.domElement) {
    renderer.domElement.remove();
  }
});
</script>

<style scoped>
.viewer-wrapper {
  padding: 1.5rem;
  display: flex;
  flex-direction: column;
  position: relative;
  overflow: hidden;
}



.canvas-container {
  flex-grow: 1;
  width: 100%;
  border-radius: 8px;
  overflow: hidden;
  background: #000;
}

.overlay {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(15, 23, 42, 0.7);
  backdrop-filter: blur(4px);
  z-index: 5;
  border-radius: 16px;
}

.overlay p {
  color: var(--accent-color);
  font-weight: 500;
  animation: pulse 1.5s infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 0.5; }
  50% { opacity: 1; }
}
</style>
