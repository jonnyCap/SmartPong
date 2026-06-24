const API_BASE = 'http://localhost:8000';

/**
 * Returns per-player metadata objects:
 *   { user_id, last_seen, total_throws, total_hits, hit_rate }
 * Sorted by most recently active player first (backend-side).
 */
export const fetchUsers = async () => {
  try {
    const response = await fetch(`${API_BASE}/users`);
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    const data = await response.json();
    return data.users || [];
  } catch (error) {
    console.error("Failed to fetch users:", error);
    return [];
  }
};

/**
 * Reset a player's stats server-side. Returns the reset confirmation or null.
 */
export const resetProfile = async (userId) => {
  try {
    const response = await fetch(`${API_BASE}/profiles/${userId}/reset`, { method: 'POST' });
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    return await response.json();
  } catch (error) {
    console.error("Failed to reset profile:", error);
    return null;
  }
};

export const fetchUserProfile = async (userId) => {
  try {
    const response = await fetch(`${API_BASE}/profiles/${userId}`);
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    const data = await response.json();
    return data;
  } catch (error) {
    console.error("Failed to fetch user profile:", error);
    return null;
  }
};
