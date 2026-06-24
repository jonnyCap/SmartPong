import { ref, watch } from 'vue';

// Whether throw metrics (peak, drift, control/drift category) are measured on
// the throw only (wind-up excluded) or over the full raw window. Persisted.
const KEY = 'smartpong.excludeWindup';
const stored = typeof localStorage !== 'undefined' ? localStorage.getItem(KEY) : null;

export const excludeWindup = ref(stored === null ? true : stored === 'true');

watch(excludeWindup, (v) => {
  try { localStorage.setItem(KEY, String(v)); } catch (_) { /* ignore */ }
});

/**
 * Pick the right metric field of a throw based on the wind-up preference.
 * Falls back to the throw-only value for older throws without "_full" data.
 */
export const peakOf = (t) =>
  (excludeWindup.value ? t?.peak_acceleration : (t?.peak_acceleration_full || t?.peak_acceleration)) || 0;

export const driftOf = (t) =>
  (excludeWindup.value ? t?.drift_angle : (t?.drift_angle_full || t?.drift_angle)) || 0;

export const controlOf = (t) =>
  (excludeWindup.value ? t?.control_category : (t?.control_category_full || t?.control_category)) || '';

export const driftCatOf = (t) =>
  (excludeWindup.value ? t?.drift_category : (t?.drift_category_full || t?.drift_category)) || '';
