import os
import time
from typing import Dict, List, Optional
from pydantic import BaseModel
from pymongo import MongoClient

class ThrowStats(BaseModel):
    peak_acceleration: float
    drift_angle: float
    is_hit: bool
    trajectory_path: List[List[float]] = []
    control_category: str = ""
    drift_category: str = ""
    # Unix epoch seconds, stamped by the backend when the throw is received.
    # Defaults to 0.0 for throws recorded before timestamps were tracked.
    timestamp: float = 0.0
    # Full-window variants (wind-up included) for the "include wind-up" toggle.
    # The primary fields above are throw-only (wind-up excluded).
    peak_acceleration_full: float = 0.0
    drift_angle_full: float = 0.0
    control_category_full: str = ""
    drift_category_full: str = ""

class UserProfile(BaseModel):
    user_id: str
    total_throws: int = 0
    total_hits: int = 0
    throws: List[ThrowStats] = []
    
    @property
    def hit_rate(self) -> float:
        if self.total_throws == 0:
            return 0.0
        return self.total_hits / self.total_throws

    @property
    def avg_peak_acceleration(self) -> float:
        if not self.throws:
            return 0.0
        return sum(t.peak_acceleration for t in self.throws) / len(self.throws)

    @property
    def std_peak_acceleration(self) -> float:
        if len(self.throws) < 2:
            return 0.0
        import numpy as np
        return float(np.std([t.peak_acceleration for t in self.throws]))

    @property
    def avg_drift_angle(self) -> float:
        if not self.throws:
            return 0.0
        return sum(t.drift_angle for t in self.throws) / len(self.throws)
        
    @property
    def std_drift_angle(self) -> float:
        if len(self.throws) < 2:
            return 0.0
        import numpy as np
        return float(np.std([t.drift_angle for t in self.throws]))

    @property
    def last_seen(self) -> float:
        """Timestamp of the most recently received throw (0.0 if none/untracked)."""
        if not self.throws:
            return 0.0
        return max((t.timestamp for t in self.throws), default=0.0)

    def summary(self) -> dict:
        """Lightweight metadata used to populate the player picker."""
        return {
            "user_id": self.user_id,
            "total_throws": self.total_throws,
            "total_hits": self.total_hits,
            "hit_rate": self.hit_rate,
            "last_seen": self.last_seen,
        }

class ProfileManager:
    def __init__(self):
        mongo_uri = os.environ.get("MONGO_URI", "mongodb://mongodb:27017/")
        self.client = MongoClient(mongo_uri)
        self.db = self.client.smartpong
        self.collection = self.db.profiles
        
    def get_profile(self, user_id: str) -> UserProfile:
        doc = self.collection.find_one({"user_id": user_id})
        if not doc:
            return UserProfile(user_id=user_id)
        return UserProfile(**doc)

    def get_all_user_ids(self) -> List[str]:
        return self.collection.distinct("user_id")

    def get_all_user_summaries(self) -> List[dict]:
        """Per-user metadata for the player picker, sorted by most recent activity."""
        summaries = [self.get_profile(uid).summary() for uid in self.get_all_user_ids()]
        summaries.sort(key=lambda s: s["last_seen"], reverse=True)
        return summaries

    def save_profile(self, profile: UserProfile):
        self.collection.update_one(
            {"user_id": profile.user_id},
            {"$set": profile.model_dump()},
            upsert=True
        )

    def reset_profile(self, user_id: str) -> UserProfile:
        """Clear a player's stats server-side, keeping the player selectable."""
        empty = UserProfile(user_id=user_id)
        self.save_profile(empty)
        return empty
        
    def add_throw(self, user_id: str, peak_accel: float, drift_angle: float, is_hit: bool, trajectory_path: List[List[float]], control_category: str, drift_category: str, timestamp: Optional[float] = None, peak_accel_full: float = 0.0, drift_angle_full: float = 0.0, control_category_full: str = "", drift_category_full: str = ""):
        profile = self.get_profile(user_id)
        profile.total_throws += 1
        if is_hit:
            profile.total_hits += 1

        profile.throws.append(ThrowStats(
            peak_acceleration=peak_accel,
            drift_angle=drift_angle,
            is_hit=is_hit,
            trajectory_path=trajectory_path,
            control_category=control_category,
            drift_category=drift_category,
            timestamp=timestamp if timestamp is not None else time.time(),
            peak_acceleration_full=peak_accel_full,
            drift_angle_full=drift_angle_full,
            control_category_full=control_category_full,
            drift_category_full=drift_category_full,
        ))
        self.save_profile(profile)
        return profile
        
    def categorize_throw_control(self, user_id: str, peak_accel: float) -> str:
        profile = self.get_profile(user_id)
        if len(profile.throws) < 5:
            # Fallback thresholds (peak g). Real throws peak around 8-12 g, so a
            # normal hard throw should land "Moderately Controlled", not "Uncontrolled".
            controlled_threshold = 7.0
            uncontrolled_threshold = 12.0
        else:
            # Once enough data exists, grade each throw relative to the player's
            # own distribution (consistency), not an absolute g value.
            mean = profile.avg_peak_acceleration
            std = profile.std_peak_acceleration
            controlled_threshold = mean - 0.5 * std
            uncontrolled_threshold = mean + 0.5 * std
            
        if peak_accel <= controlled_threshold:
            return "Controlled"
        elif peak_accel <= uncontrolled_threshold:
            return "Moderately Controlled"
        else:
            return "Uncontrolled"

    def categorize_throw_drift(self, user_id: str, drift_angle: float) -> str:
        profile = self.get_profile(user_id)
        if len(profile.throws) < 5:
            # Fallback thresholds for the path-straightness drift (degrees, ~0-45).
            # Straight throws sit near 0-10 deg; clearly curved ones above ~25.
            low_threshold = 12.0
            high_threshold = 25.0
        else:
            mean = profile.avg_drift_angle
            std = profile.std_drift_angle
            low_threshold = mean - 0.5 * std
            high_threshold = mean + 0.5 * std
            
        if drift_angle <= low_threshold:
            return "Low Drift"
        elif drift_angle <= high_threshold:
            return "Moderate Drift"
        else:
            return "Strong Drift"

profile_manager = ProfileManager()
