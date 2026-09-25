#!/usr/bin/env python3
"""Generates dashboards/desk.json, the Grafana dashboard. Edit this, not the
JSON, then run it: python3 make_dashboard.py (Grafana reloads the file)."""
import json
import os

DS = {"type": "influxdb", "uid": "influxdb"}


def flux(meas, field, agg="mean"):
    return (
        'from(bucket: "desk")\n'
        "  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)\n"
        f'  |> filter(fn: (r) => r._measurement == "{meas}" and r._field == "{field}")\n'
        f"  |> aggregateWindow(every: v.windowPeriod, fn: {agg}, createEmpty: false)"
    )


# Last 3 h of pressure: newest minus oldest reading.
PRESSURE_3H = (
    'data = from(bucket: "desk")\n'
    "  |> range(start: -3h)\n"
    '  |> filter(fn: (r) => r._measurement == "indoor" and r._field == "pressure_hpa")\n'
    "union(tables: [data |> first(), data |> last()])\n"
    '  |> sort(columns: ["_time"])\n'
    "  |> difference()"
)


def steps(base, *pairs):
    """Threshold steps: base colour below the first value, then (value, colour)."""
    return {"mode": "absolute", "steps": [{"color": base, "value": None}]
            + [{"color": c, "value": v} for v, c in pairs]}


_next_id = [0]


def panel(kind, title, series, unit, grid, desc, decimals=0, thresholds=None):
    """series: list of (display name, measurement, field) or (display name, raw flux)."""
    _next_id[0] += 1
    targets, overrides = [], []
    for i, s in enumerate(series):
        ref = chr(ord("A") + i)
        query = s[1] if len(s) == 2 else flux(s[1], s[2])
        targets.append({"refId": ref, "datasource": DS, "query": query})
        overrides.append({"matcher": {"id": "byFrameRefID", "options": ref},
                          "properties": [{"id": "displayName", "value": s[0]}]})
    defaults = {"unit": unit, "decimals": decimals}
    if thresholds:
        defaults["thresholds"] = thresholds
        defaults["color"] = {"mode": "thresholds"}
    p = {"id": _next_id[0], "type": kind, "title": title, "description": desc, "gridPos": grid,
         "datasource": DS, "targets": targets,
         "fieldConfig": {"defaults": defaults, "overrides": overrides}}
    if kind == "stat":
        p["options"] = {"reduceOptions": {"calcs": ["lastNotNull"], "fields": "", "values": False},
                        "graphMode": "area", "colorMode": "value" if thresholds else "none",
                        "textMode": "value"}
    else:
        defaults["custom"] = {"lineWidth": 2, "fillOpacity": 0, "showPoints": "never",
                              "spanNulls": 300000}
        if thresholds:
            # Faint background bands for the ranges, same colours as the tiles.
            defaults["custom"]["thresholdsStyle"] = {"mode": "area"}
            defaults["color"] = {"mode": "palette-classic"}
        p["options"] = {"legend": {"displayMode": "list", "placement": "bottom", "showLegend": True},
                        "tooltip": {"mode": "multi", "sort": "none"}}
    return p


# --- what the numbers mean (shown in each panel's (i) tooltip) ---

D_INDOOR_T = ("Room temperature from the BME280 on the display. "
              "Comfortable: 20-24 °C (green). Below 18 blue, above 25 orange, above 28 red.")
D_HUMIDITY = ("Indoor relative humidity. Comfortable: 40-60 % (green). "
              "Below 30 % dry air (orange): dry eyes/throat. "
              "Above 60 % yellow, above 70 % red: mould risk, time to air the room.")
D_PRESSURE = ("Air pressure in hPa, measured in the room (same as outside). "
              "Reads about 4 hPa below weather reports: those are converted to sea level, "
              "Berlin is ~35 m up. Rough guide (weather-report values): under 1000 low, "
              "unsettled, rain and wind likely; 1010-1020 normal; above 1025 high, settled, dry. "
              "The trend matters more than the value: see Pressure 3h.")
D_PRESSURE_3H = ("Pressure change over the last 3 hours. "
                 "Falling more than 3 hPa (orange/red): weather getting worse, rain or wind coming. "
                 "Rising more than 3 hPa (blue): clearing up. Within ±1: no change.")
D_OUTDOOR = "Current outdoor temperature in central Berlin (Open-Meteo), refreshed every 15 min."
D_AQI = ("European Air Quality Index for Berlin (Open-Meteo, refreshed every 15 min). "
         "0-20 good, 20-40 fair (green), 40-60 moderate (yellow), 60-80 poor (orange), "
         "80-100 very poor (red), above 100 extremely poor (purple).")
D_WIFI = ("WiFi signal strength at the display, in dBm: closer to 0 is stronger. "
          "-30 to -50 excellent, -50 to -67 good (green); -67 to -80 weak, "
          "occasional slow requests (orange); below -80 poor, dropouts (red). "
          "Useful when fetches fail or after moving the display.")
D_UPTIME = ("Time since the display last restarted. "
            "Drops to zero after a reset: normal after flashing or unplugging, "
            "otherwise a crash or a power problem (check the boot log for BROWNOUT).")
D_POLLEN = ("Pollen in the air, grains per m³ (Open-Meteo, Europe only). "
            "Rough guide: 1-10 low, 11-50 medium, above 50 high. "
            "Zero outside the season (about Oct-Jan).")
D_WIND_UV = ("Outdoor wind speed (km/h) and today's maximum UV index. "
             "UV: 0-2 low, 3-5 moderate, 6-7 high, 8+ very high.")
D_DEVICE = ("Display health. WiFi dBm: see the WiFi tile. "
            "Free heap: memory left on the ESP32; ~150-190 KB is normal, "
            "a steady decline over days would mean a memory leak.")

T_INDOOR = steps("blue", (18, "green"), (25, "orange"), (28, "red"))
T_HUMIDITY = steps("orange", (30, "yellow"), (40, "green"), (60, "yellow"), (70, "red"))
T_PRESSURE_3H = steps("red", (-6, "orange"), (-3, "yellow"), (-1, "green"), (1, "text"), (3, "blue"))
T_AQI = steps("green", (40, "yellow"), (60, "orange"), (80, "red"), (100, "purple"))
T_WIFI = steps("red", (-80, "orange"), (-67, "green"))

W = 3  # 8 tiles across the 24-column grid


def tile(i):
    return {"x": i * W, "y": 0, "w": W, "h": 4}


panels = [
    panel("stat", "Indoor", [("Indoor", "indoor", "temp_c")], "celsius", tile(0), D_INDOOR_T, 1, T_INDOOR),
    panel("stat", "Humidity", [("Humidity", "indoor", "humidity")], "percent", tile(1), D_HUMIDITY, 0,
          T_HUMIDITY),
    panel("stat", "Pressure", [("Pressure", "indoor", "pressure_hpa")], "pressurehpa", tile(2), D_PRESSURE),
    panel("stat", "Pressure 3h", [("Change", PRESSURE_3H)], "pressurehpa", tile(3), D_PRESSURE_3H, 1,
          T_PRESSURE_3H),
    panel("stat", "Outdoor", [("Outdoor", "outdoor", "temp_c")], "celsius", tile(4), D_OUTDOOR),
    panel("stat", "AQI", [("AQI", "air", "aqi")], "none", tile(5), D_AQI, 0, T_AQI),
    panel("stat", "WiFi signal", [("WiFi", "device", "rssi")], "dBm", tile(6), D_WIFI, 0, T_WIFI),
    panel("stat", "Uptime", [("Uptime", "device", "uptime_s")], "s", tile(7), D_UPTIME, 1),
    panel("timeseries", "Temperature", [("Indoor", "indoor", "temp_c"), ("Outdoor", "outdoor", "temp_c")],
          "celsius", {"x": 0, "y": 4, "w": 12, "h": 9}, D_INDOOR_T + " " + D_OUTDOOR, 1),
    panel("timeseries", "Indoor humidity", [("Humidity", "indoor", "humidity")], "percent",
          {"x": 12, "y": 4, "w": 12, "h": 9}, D_HUMIDITY, 0, T_HUMIDITY),
    panel("timeseries", "Pressure", [("Pressure", "indoor", "pressure_hpa")], "pressurehpa",
          {"x": 0, "y": 13, "w": 12, "h": 9}, D_PRESSURE, 1),
    panel("timeseries", "Air quality (European AQI)", [("AQI", "air", "aqi")], "none",
          {"x": 12, "y": 13, "w": 12, "h": 9}, D_AQI, 0, T_AQI),
    panel("timeseries", "Pollen (grains/m³)",
          [(n.capitalize(), "air", n) for n in ("alder", "birch", "grass", "mugwort", "ragweed")], "none",
          {"x": 0, "y": 22, "w": 12, "h": 9}, D_POLLEN),
    panel("timeseries", "Outdoor wind and UV", [("Wind km/h", "outdoor", "wind_kmh"), ("UV max", "outdoor", "uv")],
          "none", {"x": 12, "y": 22, "w": 12, "h": 9}, D_WIND_UV),
    panel("timeseries", "Device", [("WiFi dBm", "device", "rssi"), ("Free heap KB", "device", "heap_free_kb")],
          "none", {"x": 0, "y": 31, "w": 24, "h": 7}, D_DEVICE),
]

dashboard = {
    "uid": "desk", "title": "Desk display", "tags": ["esp32"], "timezone": "browser",
    "schemaVersion": 39, "refresh": "1m", "time": {"from": "now-24h", "to": "now"},
    "editable": False, "panels": panels,
    "description": "Hover the (i) next to a panel title for what its numbers mean.",
}

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "dashboards", "desk.json")
with open(out, "w") as f:
    json.dump(dashboard, f, indent=2, ensure_ascii=False)
    f.write("\n")
print(f"wrote {out}")
