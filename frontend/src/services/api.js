export const fetchUserProfile = async (userId) => {
  try {
    const response = await fetch(`http://localhost:8000/profiles/${userId}`);
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
