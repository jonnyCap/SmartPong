# SmartPong

Throw tracking for the M5StickC Plus. The device detects throws on-device and
publishes them over MQTT; the backend computes metrics and stores per-player
profiles, and the web dashboard shows them live.

## Backend + frontend

Everything (backend, frontend, MongoDB, MQTT broker) runs via Docker Compose:

```bash
docker compose up --build
```

- Dashboard: http://localhost:5173
- API: http://localhost:8000

## Firmware

Needs ESP-IDF (5.x). Set the broker host in `firmware/main/mqtt_logic.c` and
Wi-Fi credentials via `idf.py menuconfig`, then:

```bash
cd firmware
idf.py build flash monitor
```
