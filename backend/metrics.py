import numpy as np

def calculate_peak_acceleration(ax, ay, az):
    """
    Calculate peak acceleration from a window of accelerometer data.
    """
    ax, ay, az = np.asarray(ax), np.asarray(ay), np.asarray(az)
    magnitude = np.sqrt(ax**2 + ay**2 + az**2)
    return float(np.max(magnitude))

def compute_window_directions(ax, ay, az, n_windows=5):
    ax, ay, az = np.asarray(ax), np.asarray(ay), np.asarray(az)
    if len(ax) < n_windows:
        return []

    stacked = np.vstack([ax, ay, az]).T
    windows_split = np.array_split(stacked, n_windows)

    directions = []
    for w in windows_split:
        mean_vec = np.mean(w, axis=0)
        norm = np.linalg.norm(mean_vec)
        if norm < 1e-9:
            directions.append(np.array([0.0, 0.0, 0.0]))
        else:
            directions.append(mean_vec / norm)
    return directions

def angle_between_vectors(v1, v2):
    v1, v2 = np.asarray(v1), np.asarray(v2)
    n1, n2 = np.linalg.norm(v1), np.linalg.norm(v2)
    if n1 < 1e-9 or n2 < 1e-9:
        return 0.0
    dot = np.dot(v1, v2) / (n1 * n2)
    dot = np.clip(dot, -1.0, 1.0)
    return float(np.degrees(np.arccos(dot)))

def calculate_drift(ax, ay, az, n_windows=5):
    """
    Calculate the directional drift angle between the start and end of a throw.
    """
    dirs = compute_window_directions(ax, ay, az, n_windows=n_windows)
    if len(dirs) < 2:
        return 0.0
    return angle_between_vectors(dirs[0], dirs[-1])

def calculate_trajectory(ax, ay, az, gx, gy, gz, dt=0.02):
    """
    Basic trajectory calculation.
    Integrates gyroscope data for orientation and accelerometer data for position.
    Since proper sensor fusion requires a Kalman Filter or Madgwick filter,
    this is a highly simplified basic double integration for demonstration.
    """
    ax, ay, az = np.asarray(ax), np.asarray(ay), np.asarray(az)
    gx, gy, gz = np.asarray(gx), np.asarray(gy), np.asarray(gz)
    
    if len(ax) == 0:
        return {"final_position": [0.0, 0.0, 0.0], "final_rotation_deg": [0.0, 0.0, 0.0]}
    
    # Very simplified: just double integrate acceleration (assuming gravity is roughly removed or not accounted for properly in this basic version)
    # Convert g to m/s^2
    ax_m = ax * 9.81
    ay_m = ay * 9.81
    az_m = az * 9.81
    
    # Velocity
    vx = np.cumsum(ax_m * dt)
    vy = np.cumsum(ay_m * dt)
    vz = np.cumsum(az_m * dt)
    
    # Position
    px = np.cumsum(vx * dt)
    py = np.cumsum(vy * dt)
    pz = np.cumsum(vz * dt)
    
    # Calculate simple gyro integrated rotation (degrees) if provided
    if len(gx) > 0:
        rx = np.sum(gx * dt)
        ry = np.sum(gy * dt)
        rz = np.sum(gz * dt)
    else:
        rx, ry, rz = 0.0, 0.0, 0.0
    
    return {
        "final_position": [float(px[-1]), float(py[-1]), float(pz[-1])],
        "final_rotation_deg": [float(rx), float(ry), float(rz)]
    }
