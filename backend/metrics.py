import math
import numpy as np

GRAVITY_G = 9.81
MADGWICK_BETA = 0.08
MADGWICK_WARMUP = 20
THROW_CORE_FRAC = 0.40
THROW_ACTIVE_FRAC = 0.15
THROW_MARGIN = 2
THROW_MIN_ACCEL = 3.0
VIS_TARGET_EXTENT = 40.0


def _madgwick_step(q, ax, ay, az, gx, gy, gz, dt, beta):
    q0, q1, q2, q3 = q
    qDot0 = 0.5 * (-q1 * gx - q2 * gy - q3 * gz)
    qDot1 = 0.5 * ( q0 * gx + q2 * gz - q3 * gy)
    qDot2 = 0.5 * ( q0 * gy - q1 * gz + q3 * gx)
    qDot3 = 0.5 * ( q0 * gz + q1 * gy - q2 * gx)

    norm = math.sqrt(ax * ax + ay * ay + az * az)
    if norm > 1e-9:
        ax, ay, az = ax / norm, ay / norm, az / norm
        _2q0, _2q1, _2q2, _2q3 = 2 * q0, 2 * q1, 2 * q2, 2 * q3
        _4q0, _4q1, _4q2 = 4 * q0, 4 * q1, 4 * q2
        _8q1, _8q2 = 8 * q1, 8 * q2
        q0q0, q1q1, q2q2, q3q3 = q0 * q0, q1 * q1, q2 * q2, q3 * q3
        s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay
        s1 = _4q1 * q3q3 - _2q3 * ax + 4.0 * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az
        s2 = 4.0 * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az
        s3 = 4.0 * q1q1 * q3 - _2q1 * ax + 4.0 * q2q2 * q3 - _2q2 * ay
        sn = math.sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3)
        if sn > 1e-9:
            s0, s1, s2, s3 = s0 / sn, s1 / sn, s2 / sn, s3 / sn
            qDot0 -= beta * s0
            qDot1 -= beta * s1
            qDot2 -= beta * s2
            qDot3 -= beta * s3

    q0 += qDot0 * dt
    q1 += qDot1 * dt
    q2 += qDot2 * dt
    q3 += qDot3 * dt
    qn = math.sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3)
    return (q0 / qn, q1 / qn, q2 / qn, q3 / qn)


def _init_quaternion(ax0, ay0, az0):
    norm = math.sqrt(ax0 * ax0 + ay0 * ay0 + az0 * az0)
    if norm < 1e-9:
        return (1.0, 0.0, 0.0, 0.0)
    ux, uy, uz = ax0 / norm, ay0 / norm, az0 / norm
    dot = uz
    if dot > 0.999999:
        return (1.0, 0.0, 0.0, 0.0)
    if dot < -0.999999:
        return (0.0, 1.0, 0.0, 0.0)
    cx, cy, cz = uy, -ux, 0.0
    w = 1.0 + dot
    qn = math.sqrt(w * w + cx * cx + cy * cy + cz * cz)
    return (w / qn, cx / qn, cy / qn, cz / qn)


def _linear_accel_world(ax, ay, az, gx, gy, gz, dt):
    n = len(ax)
    gxr, gyr, gzr = np.radians(gx), np.radians(gy), np.radians(gz)

    q = _init_quaternion(ax[0], ay[0], az[0])
    for _ in range(MADGWICK_WARMUP):
        q = _madgwick_step(q, ax[0], ay[0], az[0], 0.0, 0.0, 0.0, dt, MADGWICK_BETA)

    lin = np.empty((n, 3))
    for i in range(n):
        q = _madgwick_step(q, ax[i], ay[i], az[i], gxr[i], gyr[i], gzr[i], dt, MADGWICK_BETA)
        w, x, y, z = q
        bx, by, bz = ax[i], ay[i], az[i]
        ex = (1 - 2 * (y * y + z * z)) * bx + 2 * (x * y - w * z) * by + 2 * (x * z + w * y) * bz
        ey = 2 * (x * y + w * z) * bx + (1 - 2 * (x * x + z * z)) * by + 2 * (y * z - w * x) * bz
        ez = 2 * (x * z - w * y) * bx + 2 * (y * z + w * x) * by + (1 - 2 * (x * x + y * y)) * bz
        lin[i, 0] = ex * GRAVITY_G
        lin[i, 1] = ey * GRAVITY_G
        lin[i, 2] = (ez - 1.0) * GRAVITY_G
    return lin


def _throw_window(lin):
    mag = np.linalg.norm(lin, axis=1)
    n = len(mag)
    if n == 0 or float(mag.max()) < THROW_MIN_ACCEL:
        return None

    k = 5
    if n >= k:
        pad = k // 2
        mag_s = np.convolve(np.pad(mag, pad, mode="edge"), np.ones(k) / k, "valid")[:n]
    else:
        mag_s = mag

    peak_val = float(mag_s.max())
    strong = np.where(mag_s >= THROW_CORE_FRAC * peak_val)[0]
    if len(strong) == 0:
        return None
    core = int(strong[-1])

    lo = THROW_ACTIVE_FRAC * peak_val
    start = core
    while start > 0 and mag_s[start - 1] > lo:
        start -= 1
    end = core
    while end < n - 1 and mag_s[end + 1] > lo:
        end += 1
    return (max(0, start - THROW_MARGIN), min(n - 1, end + THROW_MARGIN))


def _integrate_path(lin, sl, dt):
    seg = lin[sl] - lin[sl].mean(axis=0)
    vel = np.cumsum(seg * dt, axis=0)
    pos = np.cumsum(vel * dt, axis=0)

    coords = pos - pos.mean(axis=0)
    max_extent = float(np.max(np.abs(coords))) if coords.size else 0.0
    if max_extent > 1e-6:
        coords *= (VIS_TARGET_EXTENT / max_extent)
    return coords.tolist()


def _path_drift(path):
    if not path or len(path) < 3:
        return 0.0
    p = np.asarray(path, dtype=float)
    p = p - p.mean(axis=0)
    cov = np.cov(p.T)
    ev = np.linalg.eigvalsh(cov)
    l1 = float(max(ev[-1], 1e-9))
    l2 = float(max(ev[-2], 0.0))
    return float(np.degrees(np.arctan2(np.sqrt(l2), np.sqrt(l1))))


def _traj_dict(path, dt):
    return {
        "final_position": path[-1] if path else [0.0, 0.0, 0.0],
        "final_rotation_deg": [0.0, 0.0, 0.0],
        "path": path,
        "dt": dt,
    }


def analyze_throw(ax, ay, az, gx, gy, gz, dt=0.02):
    ax, ay, az = np.asarray(ax, dtype=float), np.asarray(ay, dtype=float), np.asarray(az, dtype=float)
    gx, gy, gz = np.asarray(gx, dtype=float), np.asarray(gy, dtype=float), np.asarray(gz, dtype=float)

    raw_mag = np.sqrt(ax ** 2 + ay ** 2 + az ** 2)
    peak_full = float(np.max(raw_mag)) if len(raw_mag) else 0.0

    if len(ax) < 4:
        return {"peak": peak_full, "drift": 0.0, "peak_full": peak_full,
                "drift_full": 0.0, "trajectory": _traj_dict([], dt)}

    lin = _linear_accel_world(ax, ay, az, gx, gy, gz, dt)
    full_path = _integrate_path(lin, slice(None), dt)
    drift_full = _path_drift(full_path)

    win = _throw_window(lin)
    if win is None:
        return {"peak": peak_full, "drift": drift_full, "peak_full": peak_full,
                "drift_full": drift_full, "trajectory": _traj_dict(full_path, dt)}

    start, end = win
    sl = slice(start, end + 1)
    throw_path = _integrate_path(lin, sl, dt)
    return {
        "peak": float(np.max(raw_mag[sl])),
        "drift": _path_drift(throw_path),
        "peak_full": peak_full,
        "drift_full": drift_full,
        "trajectory": _traj_dict(throw_path, dt),
    }
