<template>
  <div class="viewer-wrapper glass-panel">
    <h3>3D Trajectory Viewer</h3>
    <div class="canvas-container" ref="canvasContainer"></div>
    <div v-if="!latestTrajectory" class="overlay">
      <p>Waiting for throw data...</p>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, watch } from 'vue';
import * as THREE from 'three';

const props = defineProps({
  latestTrajectory: {
    type: Object,
    default: null
  }
});

const canvasContainer = ref(null);
let scene, camera, renderer, animationId;
let throwPath, ballMesh;

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
    
    // Slow rotation of camera for cinematic effect
    if (camera) {
      camera.position.x = Math.sin(Date.now() * 0.0005) * 10;
      camera.position.z = Math.cos(Date.now() * 0.0005) * 10;
      camera.lookAt(0, 0, 0);
    }
    
    renderer.render(scene, camera);
  };
  animate();

  // Resize handler
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

const updateTrajectory = (trajectory) => {
  if (!trajectory || !trajectory.final_position) return;
  
  const [px, py, pz] = trajectory.final_position;
  
  // Remove old path if exists
  if (throwPath) {
    scene.remove(throwPath);
  }

  // Create simple straight path for visualization (as we only get final position from basic backend)
  const material = new THREE.LineBasicMaterial({ 
    color: 0x10B981,
    linewidth: 2 
  });
  
  const points = [];
  points.push(new THREE.Vector3(0, 0, 0));
  // Scale down position for viewer
  const endPoint = new THREE.Vector3(px * 0.1, py * 0.1, pz * 0.1);
  points.push(endPoint);

  const geometry = new THREE.BufferGeometry().setFromPoints(points);
  throwPath = new THREE.Line(geometry, material);
  scene.add(throwPath);

  // Animate ball moving to end position
  const startPos = new THREE.Vector3(0, 0, 0);
  let progress = 0;
  
  const moveBall = () => {
    progress += 0.02;
    if (progress <= 1) {
      ballMesh.position.lerpVectors(startPos, endPoint, progress);
      requestAnimationFrame(moveBall);
    }
  };
  moveBall();
};

watch(() => props.latestTrajectory, (newTraj) => {
  if (newTraj) {
    updateTrajectory(newTraj);
  }
}, { deep: true });

onMounted(() => {
  initThreeJS();
  if (props.latestTrajectory) {
    updateTrajectory(props.latestTrajectory);
  }
});

onUnmounted(() => {
  window.removeEventListener('resize', onWindowResize);
  if (animationId) cancelAnimationFrame(animationId);
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

h3 {
  color: var(--text-muted);
  font-size: 1rem;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  margin-bottom: 1rem;
  z-index: 10;
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
