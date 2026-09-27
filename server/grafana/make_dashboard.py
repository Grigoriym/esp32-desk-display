#!/usr/bin/env python3
"""Generates dashboards/desk.json, the Grafana dashboard. Edit this, not the
JSON, then run it: python3 make_dashboard.py (Grafana reloads the file)."""
import json
import os

DS = {"type": "influxdb", "uid": "influxdb"}

# The display uploads every 60 s: no reading for longer than this = offline.
ONLINE_S = 180


def flux(meas, field, agg="mean", empty=False):
    """empty=True: windows with no data come back as nulls, so a graph can
    break its line while the display was off (see spanNulls in graph())."""
    return (
        'from(bucket: "desk")\n'
        "  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)\n"
        f'  |> filter(fn: (r) => r._measurement == "{meas}" and r._field == "{field}")\n'
        f"  |> aggregateWindow(every: v.windowPeriod, fn: {agg}, createEmpty: {str(empty).lower()})"
    )


def words(ranges, top):
    """Flux that sets `level` from the last value of `data`: the word of the
    first (upper bound, word) the value is below, else top. Same bounds as the
    colour thresholds, so word and colour always agree."""
    chain = "".join(f'if x[0] < {float(v)} then "{w}" else ' for v, w in ranges)
    return ('x = data |> last() |> findColumn(fn: (key) => true, column: "_value")\n'
            f'level = if length(arr: x) == 0 then "" else {chain}"{top}"')


def labelled(query, level, prelude=""):
    """A tile query whose series carries a `level` label (the word shown above
    the number, via displayName ${__field.labels.level})."""
    return (f"{prelude}data = {query}\n{level}\n"
            'data |> set(key: "level", value: level)\n'
            '  |> group(columns: ["_start", "_stop", "_measurement", "_field", "device", "level"])')


# Last 3 h of pressure: newest minus oldest reading.
PRESSURE_3H_PRELUDE = (
    'raw = from(bucket: "desk")\n'
    "  |> range(start: -3h)\n"
    '  |> filter(fn: (r) => r._measurement == "indoor" and r._field == "pressure_hpa")\n'
)
PRESSURE_3H = 'union(tables: [raw |> first(), raw |> last()])\n  |> sort(columns: ["_time"])\n  |> difference()'

# Open-Meteo's WMO weather codes, as words.
WEATHER = {0: "Clear", 1: "Mostly clear", 2: "Partly cloudy", 3: "Overcast", 45: "Fog", 48: "Fog",
           51: "Drizzle", 53: "Drizzle", 55: "Drizzle", 56: "Freezing drizzle", 57: "Freezing drizzle",
           61: "Light rain", 63: "Rain", 65: "Heavy rain", 66: "Freezing rain", 67: "Freezing rain",
           71: "Light snow", 73: "Snow", 75: "Heavy snow", 77: "Snow grains",
           80: "Showers", 81: "Showers", 82: "Heavy showers", 85: "Snow showers", 86: "Snow showers",
           95: "Thunderstorm", 96: "Thunderstorm, hail", 99: "Thunderstorm, hail"}
WEATHER_LEVEL = (
    'code = from(bucket: "desk")\n'
    "  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)\n"
    '  |> filter(fn: (r) => r._measurement == "outdoor" and r._field == "weather_code")\n'
    "  |> last()\n"
    '  |> findColumn(fn: (key) => true, column: "_value")\n'
    'level = if length(arr: code) == 0 then "" else dict.get(dict: ['
    + ", ".join(f'{k}: "{v}"' for k, v in WEATHER.items())
    + '], key: int(v: code[0]), default: "")'
)

# Seconds since the last upload, labelled "Display" while online.
LAST_SEEN = (
    'from(bucket: "desk")\n'
    "  |> range(start: -30d)\n"
    '  |> filter(fn: (r) => r._measurement == "device" and r._field == "uptime_s")\n'
    "  |> last()\n"
    "  |> map(fn: (r) => ({_time: r._time, _value: float(v: int(v: now()) - int(v: r._time)) / 1000000000.0}))\n"
    f'  |> map(fn: (r) => ({{r with level: if r._value < {ONLINE_S}.0 then "Display" else "Offline for"}}))\n'
    '  |> group(columns: ["level"])'
)

# Boot moments: where uptime drops, minus the new uptime.
RESTARTS = (
    'from(bucket: "desk")\n'
    "  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)\n"
    '  |> filter(fn: (r) => r._measurement == "device" and r._field == "uptime_s")\n'
    '  |> duplicate(column: "_value", as: "up")\n'
    "  |> difference()\n"
    "  |> filter(fn: (r) => r._value < 0.0)\n"
    '  |> map(fn: (r) => ({_time: time(v: int(v: r._time) - int(v: r.up) * 1000000000), text: "Display restarted"}))'
)


def pollen(field):
    """Hourly pollen, dropped entirely when it's zero over the whole range."""
    return (f"data = {flux('air', field, 'max', empty=True)}\n"
            'peak = data |> max() |> findColumn(fn: (key) => true, column: "_value")\n'
            "data |> filter(fn: (r) => length(arr: peak) > 0 and peak[0] > 0.0)")


def steps(base, *pairs):
    """Threshold steps: base colour below the first value, then (value, colour)."""
    return {"mode": "absolute", "steps": [{"color": base, "value": None}]
            + [{"color": c, "value": v} for v, c in pairs]}


_next_id = [0]


def _base(kind, title, targets, overrides, grid, desc, defaults):
    _next_id[0] += 1
    return {"id": _next_id[0], "type": kind, "title": title, "description": desc, "gridPos": grid,
            "datasource": DS, "targets": targets,
            "fieldConfig": {"defaults": defaults, "overrides": overrides}}


def stat(title, query, unit, grid, desc, decimals=0, thresholds=None, level=True, sparkline=True,
         mappings=None):
    """One number; level=True: the query carries a `level` word shown above it."""
    defaults = {"unit": unit, "decimals": decimals}
    if thresholds:
        defaults["thresholds"] = thresholds
        defaults["color"] = {"mode": "thresholds"}
    if mappings:
        defaults["mappings"] = mappings
    defaults["displayName"] = "${__field.labels.level}" if level else title
    p = _base("stat", title, [{"refId": "A", "datasource": DS, "query": query}], [], grid, desc, defaults)
    p["options"] = {"reduceOptions": {"calcs": ["lastNotNull"], "fields": "", "values": False},
                    "graphMode": "area" if sparkline else "none",
                    "colorMode": "value" if thresholds else "none",
                    "textMode": "value_and_name" if level else "value"}
    return p


def graph(title, series, unit, grid, desc, decimals=0, thresholds=None, props=None, no_value=None):
    """series: list of (display name, measurement, field) or (display name, raw flux).
    props: {series index: [extra field properties]}, e.g. a second axis."""
    targets, overrides = [], []
    for i, s in enumerate(series):
        ref = chr(ord("A") + i)
        query = s[1] if len(s) == 2 else flux(s[1], s[2], empty=True)
        targets.append({"refId": ref, "datasource": DS, "query": query})
        overrides.append({"matcher": {"id": "byFrameRefID", "options": ref},
                          "properties": [{"id": "displayName", "value": s[0]}] + (props or {}).get(i, [])})
    # Bridge nulls only across gaps under 5 min (uploads are every 60 s):
    # a longer gap is the display being off, and the line breaks there.
    custom = {"lineWidth": 2, "fillOpacity": 0, "showPoints": "never", "spanNulls": 300000}
    defaults = {"unit": unit, "decimals": decimals, "custom": custom}
    if thresholds:
        # Faint background bands for the ranges, same colours as the tiles.
        defaults["thresholds"] = thresholds
        custom["thresholdsStyle"] = {"mode": "area"}
        defaults["color"] = {"mode": "palette-classic"}
    if no_value:
        defaults["noValue"] = no_value
    p = _base("timeseries", title, targets, overrides, grid, desc, defaults)
    p["options"] = {"legend": {"displayMode": "list", "placement": "bottom", "showLegend": True,
                               "calcs": ["min", "max", "mean"]},
                    "tooltip": {"mode": "multi", "sort": "none"}}
    return p


def row(title, y, collapsed=False, children=()):
    """Section header. A collapsed row holds its panels; an open one is
    followed by them."""
    _next_id[0] += 1
    return {"id": _next_id[0], "type": "row", "title": title, "collapsed": collapsed,
            "gridPos": {"x": 0, "y": y, "w": 24, "h": 1}, "panels": list(children)}


def right_axis(unit, label, **extra):
    return ([{"id": "unit", "value": unit}, {"id": "custom.axisPlacement", "value": "right"},
             {"id": "custom.axisLabel", "value": label}]
            + [{"id": f"custom.{k}", "value": v} for k, v in extra.items()])


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
              "The word on the tile uses those, shifted down 4 hPa. "
              "The trend matters more than the value: see Pressure 3h.")
D_PRESSURE_3H = ("Pressure change over the last 3 hours. "
                 "Falling more than 3 hPa (orange/red): weather getting worse, rain or wind coming. "
                 "Rising more than 3 hPa (blue): clearing up. Within ±1: no change.")
D_CO2 = ("CO2 in the room, in ppm, from the SCD41 on the display (one reading per 30 s). "
         "Outdoor air is about 420. Below 800 fresh (green); 800-1000 fine (yellow); "
         "1000-1400 stuffy, concentration drops, time to air the room (orange); "
         "above 1400 open a window now (red). Breathing on the sensor makes it jump.")
D_OUTDOOR = ("Current outdoor temperature and weather in central Berlin (Open-Meteo), "
             "refreshed every 15 min.")
D_AQI = ("European Air Quality Index for Berlin (Open-Meteo, refreshed every 15 min). "
         "0-20 good, 20-40 fair (green), 40-60 moderate (yellow), 60-80 poor (orange), "
         "80-100 very poor (red), above 100 extremely poor (purple).")
D_WIFI = ("WiFi signal strength at the display, in dBm: closer to 0 is stronger. "
          "-30 to -50 excellent, -50 to -67 good (green); -67 to -80 weak, "
          "occasional slow requests (orange); below -80 poor, dropouts (red). "
          "Useful when fetches fail or after moving the display.")
D_STATUS = (f"Online while the display uploads (every 60 s). No upload for {ONLINE_S // 60} min: "
            "shows how long it has been off. Graphs have a gap there, and a marker where it "
            "came back (Display restarted).")
D_UPTIME = ("Time since the display last restarted. "
            "Drops to zero after a reset: normal after flashing or unplugging, "
            "otherwise a crash or a power problem (check the boot log for BROWNOUT). "
            "Each restart is a marker on the graphs.")
D_POLLEN = ("Pollen in the air, grains per m³ (Open-Meteo, Europe only). "
            "Bands: 1-10 low, 11-50 medium (yellow), above 50 high (red). "
            "Types at zero over the whole time range are left out; "
            "zero outside the season (about Oct-Jan).")
D_WIND_UV = ("Outdoor wind speed (km/h, left axis) and today's maximum UV index (right axis). "
             "UV: 0-2 low, 3-5 moderate, 6-7 high, 8+ very high.")
D_DEVICE = ("Display health. WiFi dBm (left axis): see the WiFi tile. "
            "Free heap (right axis): memory left on the ESP32; ~150-190 KB is normal, "
            "a steady decline over days would mean a memory leak.")

T_INDOOR = steps("blue", (18, "green"), (25, "orange"), (28, "red"))
T_HUMIDITY = steps("orange", (30, "yellow"), (40, "green"), (60, "yellow"), (70, "red"))
T_PRESSURE_3H = steps("red", (-6, "orange"), (-3, "yellow"), (-1, "green"), (1, "text"), (3, "blue"))
T_CO2 = steps("green", (800, "yellow"), (1000, "orange"), (1400, "red"))
T_AQI = steps("green", (40, "yellow"), (60, "orange"), (80, "red"), (100, "purple"))
T_WIFI = steps("red", (-80, "orange"), (-67, "green"))
T_POLLEN = steps("transparent", (11, "yellow"), (51, "red"))
T_STATUS = steps("green", (ONLINE_S, "red"))

W_INDOOR = words([(18, "Cold"), (20, "Cool"), (25, "Comfortable"), (28, "Warm")], "Hot")
W_HUMIDITY = words([(30, "Dry"), (40, "A bit dry"), (60, "Comfortable"), (70, "Humid")], "Too humid")
W_PRESSURE = words([(996, "Low"), (1021, "Normal")], "High")
W_PRESSURE_3H = words([(-6, "Falling fast"), (-3, "Falling"), (-1, "Falling a bit"), (1, "Steady"),
                       (3, "Rising a bit")], "Rising")
W_CO2 = words([(800, "Fresh"), (1000, "Fine"), (1400, "Stuffy")], "Open a window")
W_AQI = words([(20, "Good"), (40, "Fair"), (60, "Moderate"), (80, "Poor"), (100, "Very poor")],
              "Extremely poor")
W_WIFI = words([(-80, "Poor"), (-67, "Weak"), (-50, "Good")], "Excellent")

W = 3  # 8 tiles across the 24-column grid


def tile(i):
    return {"x": i * W, "y": 0, "w": W, "h": 4}


def tile_query(meas, field, level):
    return labelled(flux(meas, field), level)


panels = [
    stat("Indoor", tile_query("indoor", "temp_c", W_INDOOR), "celsius", tile(0), D_INDOOR_T, 1, T_INDOOR),
    stat("Humidity", tile_query("indoor", "humidity", W_HUMIDITY), "percent", tile(1), D_HUMIDITY, 0,
         T_HUMIDITY),
    stat("Pressure", tile_query("indoor", "pressure_hpa", W_PRESSURE), "pressurehpa", tile(2), D_PRESSURE),
    stat("Pressure 3h", labelled(PRESSURE_3H, W_PRESSURE_3H, PRESSURE_3H_PRELUDE), "pressurehpa", tile(3),
         D_PRESSURE_3H, 1, T_PRESSURE_3H),
    stat("Outdoor", labelled(flux("outdoor", "temp_c"), WEATHER_LEVEL, 'import "dict"\n'), "celsius", tile(4),
         D_OUTDOOR),
    stat("AQI", tile_query("air", "aqi", W_AQI), "none", tile(5), D_AQI, 0, T_AQI),
    stat("WiFi signal", tile_query("device", "rssi", W_WIFI), "dBm", tile(6), D_WIFI, 0, T_WIFI),
    stat("Display", LAST_SEEN, "dtdurations", tile(7), D_STATUS, 2, T_STATUS, sparkline=False,
         mappings=[{"type": "range", "options": {"from": 0, "to": ONLINE_S,
                                                 "result": {"text": "Online", "color": "green"}}}]),

    row("Room", 4),
    stat("CO2", tile_query("co2", "ppm", W_CO2), "ppm", {"x": 0, "y": 5, "w": 4, "h": 8}, D_CO2, 0, T_CO2),
    graph("CO2", [("CO2", "co2", "ppm")], "ppm", {"x": 4, "y": 5, "w": 20, "h": 8}, D_CO2, 0, T_CO2),
    graph("Temperature", [("Indoor", "indoor", "temp_c"), ("Outdoor", "outdoor", "temp_c")],
          "celsius", {"x": 0, "y": 13, "w": 12, "h": 9}, D_INDOOR_T + " " + D_OUTDOOR, 1),
    graph("Indoor humidity", [("Humidity", "indoor", "humidity")], "percent",
          {"x": 12, "y": 13, "w": 12, "h": 9}, D_HUMIDITY, 0, T_HUMIDITY),
    graph("Pressure", [("Pressure", "indoor", "pressure_hpa")], "pressurehpa",
          {"x": 0, "y": 22, "w": 24, "h": 8}, D_PRESSURE, 1),

    row("Outside", 30),
    graph("Air quality (European AQI)", [("AQI", "air", "aqi")], "none",
          {"x": 0, "y": 31, "w": 12, "h": 9}, D_AQI, 0, T_AQI),
    graph("Pollen (grains/m³)",
          [(n.capitalize(), pollen(n)) for n in ("alder", "birch", "grass", "mugwort", "ragweed")], "none",
          {"x": 12, "y": 31, "w": 12, "h": 9}, D_POLLEN, 0, T_POLLEN, no_value="No pollen in this time range"),
    graph("Outdoor wind and UV", [("Wind", "outdoor", "wind_kmh"), ("UV max", "outdoor", "uv")],
          "velocitykmh", {"x": 0, "y": 40, "w": 24, "h": 8}, D_WIND_UV,
          props={1: right_axis("none", "UV index", lineInterpolation="stepAfter")}),

    # Debugging only: collapsed so it doesn't take space day to day.
    row("Device", 48, collapsed=True, children=[
        stat("Uptime", flux("device", "uptime_s"), "dtdurations", {"x": 0, "y": 49, "w": 4, "h": 7}, D_UPTIME,
             2, level=False),
        graph("Device", [("WiFi", "device", "rssi"), ("Free heap", "device", "heap_free_kb")], "dBm",
              {"x": 4, "y": 49, "w": 20, "h": 7}, D_DEVICE,
              props={1: right_axis("kbytes", "Free heap")}),
    ]),
]

dashboard = {
    "uid": "desk", "title": "Desk display", "tags": ["esp32"], "timezone": "Europe/Berlin",
    "schemaVersion": 39, "refresh": "1m", "time": {"from": "now-24h", "to": "now"},
    "editable": False, "panels": panels,
    "annotations": {"list": [{"name": "Restarts", "datasource": DS, "enable": True,
                              "iconColor": "rgba(255, 152, 48, 0.7)",
                              "target": {"refId": "Anno", "query": RESTARTS}}]},
    "description": "Hover the (i) next to a panel title for what its numbers mean.",
}

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "dashboards", "desk.json")
with open(out, "w") as f:
    json.dump(dashboard, f, indent=2, ensure_ascii=False)
    f.write("\n")
print(f"wrote {out}")
