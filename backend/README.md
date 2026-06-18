# SmartPong Backend

This is the FastAPI backend for the SmartPong application. It is responsible for receiving throw data from the M5 watch via MQTT, analyzing the throw to determine control category, drift category, and a rudimentary trajectory, and maintaining user statistics (such as hit rates and average throw parameters).

## Architecture

1. **Firmware (M5 Watch)**: Uses the decision tree to classify a throw vs noise. When a throw is detected and the user indicates if it was a hit or miss, the firmware packages the window of accelerometer and gyroscope data and sends it via MQTT.
2. **Backend**: Subscribes to the MQTT topic `/smartpong/throws/data`. When it receives data, it computes metrics and categorizes the throw based on the specific user's historical profile. It replies via MQTT on `/smartpong/throws/results`.

## Setup Instructions

1. **Install dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

2. **Run the backend:**
   ```bash
   uvicorn main:app --reload --host 0.0.0.0 --port 8000
   ```
   *Note: This starts the HTTP server for profile viewing and the MQTT client in the background.*

## MQTT Communication

### Topics
- **Input Topic (Subscribe)**: `/smartpong/throws/data`
- **Output Topic (Publish)**: `/smartpong/throws/results`

### Expected Input Payload (JSON)
The payload published by the watch to `/smartpong/throws/data` should match this structure:
```json
{
  "user_id": "player1",
  "hit": true,
  "ax": [0.1, 0.2, ...],
  "ay": [0.0, -0.1, ...],
  "az": [0.9, 0.95, ...],
  "gx": [0.0, 0.1, ...],
  "gy": [0.1, -0.1, ...],
  "gz": [0.0, 0.0, ...]
}
```

### Result Payload (JSON)
The payload sent back to `/smartpong/throws/results` by the backend:
```json
{
  "user_id": "player1",
  "peak_acceleration": 2.5,
  "control_category": "Moderately Controlled",
  "drift_angle": 12.3,
  "drift_category": "Low Drift",
  "trajectory": {
    "final_position": [1.2, 0.5, 0.1],
    "final_rotation_deg": [15.0, -5.0, 0.0]
  },
  "user_total_throws": 10,
  "user_hit_rate": 0.6
}
```

## Agent Development

Agents maintaining or extending this code should consider:
- `metrics.py`: The `calculate_trajectory` function is currently a simplified double-integrator for demonstration. If the project gathers accurate gyroscope data, a Kalman filter or Madgwick filter could be added here.
- `profiles.py`: User stats are currently stored in memory via a python dictionary. If persistence across reboots is needed, replacing the `ProfileManager`'s dict with an SQLite database (using SQLAlchemy) would be the first step.
- `main.py`: Currently connects to `test.mosquitto.org`. If deploying to production, this broker address should be moved to environment variables and pointed to an internal/secured broker.
