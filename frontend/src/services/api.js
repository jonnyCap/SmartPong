const API_BASE = import.meta.env.VITE_API_BASE || `http://${window.location.hostname}:8000`;

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
