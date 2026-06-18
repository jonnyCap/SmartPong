from fastapi import FastAPI
from fastapi_mqtt import FastMQTT, MQTTConfig
from pydantic import BaseModel
import json

from metrics import calculate_peak_acceleration, calculate_drift, calculate_trajectory
from profiles import profile_manager

app = FastAPI(title="SmartPong Backend")

# We use test.mosquitto.org as a public broker for prototyping.
# The ESP32 firmware config should match this broker and these topics.
mqtt_config = MQTTConfig(
    host="test.mosquitto.org",
    port=1883,
    keepalive=60,
)

fast_mqtt = FastMQTT(config=mqtt_config)
fast_mqtt.init_app(app)

class ThrowPayload(BaseModel):
    user_id: str
    hit: bool
    ax: list[float]
    ay: list[float]
    az: list[float]
    gx: list[float]
    gy: list[float]
    gz: list[float]

@fast_mqtt.on_connect()
def connect(client, flags, rc, properties):
    # Topic to receive classified throw windows from the ESP32
    fast_mqtt.client.subscribe("/smartpong/throws/data")
    print("Connected to MQTT Broker!")

@fast_mqtt.on_message()
async def message(client, topic, payload, qos, properties):
    if topic == "/smartpong/throws/data":
        try:
            data = json.loads(payload.decode())
            payload_obj = ThrowPayload(**data)
            
            # 1. Calculate Metrics
            peak_accel = calculate_peak_acceleration(payload_obj.ax, payload_obj.ay, payload_obj.az)
            drift_angle = calculate_drift(payload_obj.ax, payload_obj.ay, payload_obj.az)
            trajectory = calculate_trajectory(
                payload_obj.ax, payload_obj.ay, payload_obj.az,
                payload_obj.gx, payload_obj.gy, payload_obj.gz
            )
            
            # 2. Categorize based on user profile
            control_category = profile_manager.categorize_throw_control(payload_obj.user_id, peak_accel)
            drift_category = profile_manager.categorize_throw_drift(payload_obj.user_id, drift_angle)
            
            # 3. Add to profile and update stats
            profile = profile_manager.add_throw(
                user_id=payload_obj.user_id,
                peak_accel=peak_accel,
                drift_angle=drift_angle,
                is_hit=payload_obj.hit
            )
            
            # 4. Return results to the watch
            result = {
                "user_id": payload_obj.user_id,
                "peak_acceleration": peak_accel,
                "control_category": control_category,
                "drift_angle": drift_angle,
                "drift_category": drift_category,
                "trajectory": trajectory,
                "user_total_throws": profile.total_throws,
                "user_hit_rate": profile.hit_rate
            }
            
            fast_mqtt.publish("/smartpong/throws/results", json.dumps(result))
            print(f"Processed throw for {payload_obj.user_id}. Results published to /smartpong/throws/results")
            
        except Exception as e:
            print(f"Error processing MQTT message: {e}")

@app.get("/")
def read_root():
    return {"message": "SmartPong Backend is running."}

@app.get("/profiles/{user_id}")
def get_user_profile(user_id: str):
    """
    Retrieve user stats such as average peak acceleration and hit rate.
    """
    return profile_manager.get_profile(user_id)
