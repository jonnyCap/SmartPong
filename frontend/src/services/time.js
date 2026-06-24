/**
 * Human-friendly relative time from a Unix epoch (seconds).
 * Returns "—" for missing/untracked timestamps (0 / falsy).
 */
export const formatRelative = (epochSeconds, nowMs = Date.now()) => {
  if (!epochSeconds) return '—';
  const diffSec = Math.max(0, Math.round(nowMs / 1000 - epochSeconds));

  if (diffSec < 5) return 'just now';
  if (diffSec < 60) return `${diffSec}s ago`;

  const min = Math.floor(diffSec / 60);
  if (min < 60) return `${min}m ago`;

  const hours = Math.floor(min / 60);
  if (hours < 24) return `${hours}h ago`;

  const days = Math.floor(hours / 24);
  if (days < 30) return `${days}d ago`;

  return new Date(epochSeconds * 1000).toLocaleDateString('en-US');
};

/**
 * Absolute, locale-formatted timestamp for tooltips.
 */
export const formatAbsolute = (epochSeconds) => {
  if (!epochSeconds) return 'No data received yet';
  return new Date(epochSeconds * 1000).toLocaleString('en-US');
};

/**
 * A player counts as "live" if their last throw arrived within this window.
 */
export const ONLINE_WINDOW_SEC = 45;

export const isOnline = (epochSeconds, nowMs = Date.now()) => {
  if (!epochSeconds) return false;
  return nowMs / 1000 - epochSeconds <= ONLINE_WINDOW_SEC;
};
