import { ref, watch } from 'vue';

const KEY = 'smartpong.excludeWindup';
const stored = typeof localStorage !== 'undefined' ? localStorage.getItem(KEY) : null;

export const excludeWindup = ref(stored === null ? true : stored === 'true');

watch(excludeWindup, (v) => {
  try { localStorage.setItem(KEY, String(v)); } catch (_) { /* noop */ }
});

export const peakOf = (t) =>
  (excludeWindup.value ? t?.peak_acceleration : (t?.peak_acceleration_full || t?.peak_acceleration)) || 0;

export const driftOf = (t) =>
  (excludeWindup.value ? t?.drift_angle : (t?.drift_angle_full || t?.drift_angle)) || 0;

export const controlOf = (t) =>
  (excludeWindup.value ? t?.control_category : (t?.control_category_full || t?.control_category)) || '';

export const driftCatOf = (t) =>
  (excludeWindup.value ? t?.drift_category : (t?.drift_category_full || t?.drift_category)) || '';
