# Dashboard server

InfluxDB (stores the readings) + Grafana (the dashboard page), in Docker
Compose. The display POSTs indoor, outdoor, air-quality and device values
every minute (`main/metrics.c`); Grafana shows them as graphs over any time
range.

Run it on an **always-on machine on the display's WiFi/LAN**: the ESP32
talks plain HTTP to it and can't reach Tailscale.

## Setup

Full step-by-step (prerequisites, verification, firewall, what to hand
back) for setting it up on another machine: **`INSTALL.md`**. Short
version:

1. Copy this `server/` folder to the machine, then in it:

   ```sh
   cp .env.example .env
   # INFLUX_TOKEN: openssl rand -hex 32; INFLUX_ADMIN_PASSWORD: anything
   docker compose up -d
   ```

2. Firmware: `cp main/metrics_secrets.h.example main/metrics_secrets.h`,
   set `METRICS_HOST` to the machine's LAN IP and `METRICS_TOKEN` to the
   same `INFLUX_TOKEN`, build and flash. The boot log says
   `metrics: uploading to <ip>`, then `metrics: upload OK` about a minute
   later (or `upload failed: ...`, logged once until it recovers).

3. Open `http://<machine>:34897`: the "Desk display" dashboard is the home
   page, no login needed (read-only). Log in as `admin` /
   `INFLUX_ADMIN_PASSWORD` to change things; the provisioned dashboard
   itself is read-only, edit `grafana/dashboards/desk.json` instead
   (Grafana reloads it).

Give the machine a fixed LAN IP (DHCP reservation in the router), or the
display loses it after a router restart.

## Data

Bucket `desk`, org `desk`, kept forever (a few KB a day). Measurements,
all fields floats, tag `device=desk`:

| Measurement | Fields |
|---|---|
| `indoor` | `temp_c`, `humidity`, `pressure_hpa` (BME280, 10 s old at most) |
| `outdoor` | `temp_c`, `wind_kmh`, `uv`, `weather_code` (Open-Meteo, refreshed every 15 min) |
| `air` | `aqi`, `alder`, `birch`, `grass`, `mugwort`, `ragweed` (same cadence) |
| `device` | `uptime_s`, `heap_free_kb`, `rssi` |

A measurement only appears once the display has data for it. Timestamps are
set by InfluxDB on arrival.

InfluxDB's own UI (`http://<machine>:34898`, `admin` / password) has a data
explorer for ad-hoc queries.
