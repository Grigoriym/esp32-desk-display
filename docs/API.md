# Desk display HTTP API

The contract between the desk display (this repo's ESP32 firmware) and
clients such as the phone app. Everything a client needs to find the
display, read its data and control it is here. The Android side is out of
scope.

- Example response: [`docs/api/status.example.json`](api/status.example.json).
  A host test (`test_matches_documented_example` in `test/test_web_api.c`)
  checks that the firmware produces exactly that file for the matching data,
  so the file and the firmware can't drift apart. Use it as the client's
  parsing fixture.
- Firmware side: `main/web.c` (server, mDNS), `main/web_api.c` (JSON,
  command parsing).

## Network

- **Home WiFi (LAN) only.** The ESP32 isn't reachable from outside the
  home network (no Tailscale on it, no port forwarding).
- **Plain HTTP on port 80**, no TLS. Android blocks cleartext HTTP by
  default, so the app has to allow cleartext traffic (for local/LAN hosts
  at least).
- **No authentication.** Anyone on the LAN can read and control it.
- The display gets its IP by DHCP (currently `192.168.0.147`, but it can
  change), so a client should find it by discovery rather than a fixed IP.

## Finding the display

Two ways, both provided by the firmware's mDNS responder:

| Method | Value |
|---|---|
| **DNS-SD / NSD service discovery** (preferred on Android: `NsdManager`) | service type `_http._tcp.`, service name **`Desk display`**, port **80**, no TXT records |
| **mDNS hostname** | `desk.local` → `http://desk.local/` |

Not every Android version resolves `.local` hostnames through the normal
DNS path, so use NSD discovery (then resolve the service to get host +
port) and keep a manual IP entry as a fallback. Filter discovered
`_http._tcp.` services by the service name `Desk display`: other devices
(printers, routers) announce `_http._tcp.` too.

## Endpoints

| Method + path | Purpose |
|---|---|
| `GET /api/status` | Everything the display shows, as JSON |
| `POST /api/screen?go=<value>` | Switch the screen shown on the panel |
| `POST /api/panel?set=<value>` | Turn the panel (OLED) on or off |
| `GET /` | The display's own small web page (not needed by the app) |

Parameters go in the **query string**; POST bodies are ignored and can be
empty.

### `GET /api/status`

`200`, `Content-Type: application/json`, `Cache-Control: no-store`, about
1 KB. Full example in [`api/status.example.json`](api/status.example.json).

Top level:

| Field | Type | Meaning |
|---|---|---|
| `time` | string `"HH:MM"` | Display's local time (Europe/Berlin, DST handled), when the response was built |
| `date` | string `"YYYY-MM-DD"` | Display's local date. Right after a cold boot without network/RTC it can be 1970 until the clock syncs |
| `screen` | string | Screen currently on the panel: `home`, `outdoor`, `air`, `indoor`, `bvg` |
| `panel_on` | bool | Whether the OLED is on (off = dark, the display still runs and fetches) |
| `outdoor` | object or `null` | Weather outside, see below |
| `indoor` | object or `null` | Room sensor (BME280), see below |
| `air` | object or `null` | Air quality and pollen, see below |
| `warning` | object or `null` | DWD weather warnings, see below |
| `next_holiday` | object or `null` | Next Berlin public holiday, see below |
| `bvg` | object or `null` | Next U-Bahn departures, see below |

**`null` means "no data yet"**: the first fetch or sensor read hasn't
succeeded since boot (the panel shows `--` then). Once a section has data
it stays, and a later failed fetch keeps the last good values. Exception:
`warning`, which goes back to `null` when a fetch fails (a stale warning
is worse than none). Clients should render every section independently and
handle `null` for each.

There are **no timestamps per section**; how old the data can be follows
from the refresh cadence:

| Section | Source | Refreshed |
|---|---|---|
| `outdoor`, `air`, `warning` | Open-Meteo, Open-Meteo air quality, DWD via Bright Sky | every 15 min (sooner retries after a failure) |
| `indoor` | BME280 on the device | every 10 s |
| `bvg` | BVG departures (community API, **often down**, see below) | every 60 s, but only while fetching is active (see `bvg`) |
| `next_holiday` | Nager.Date | once a year |

#### `outdoor`

| Field | Type | Meaning |
|---|---|---|
| `temp_c` | int | Current outdoor temperature, °C, rounded. A model value for central Berlin (Open-Meteo), not a station measurement: can read ~1-2 °C off on sunny mornings |
| `weather_code` | int | WMO weather code (Open-Meteo `weathercode`). The panel groups it as: 0-1 sun; 2, 3, 45, 48 cloud; 51-57, 61-67, 80-82 rain; 71-77, 85-86 snow; 95-99 storm; anything else cloud |
| `wind_kmh` | int | Current wind speed, km/h, rounded |
| `uv_max` | int | Today's maximum UV index, rounded |
| `sunrise`, `sunset` | string `"HH:MM"` | Today's, local time |
| `rain.in_h` | int | Hours until the first hour with ≥ 50% chance of rain, within the next 12 h: `0` = raining now (this hour), `-1` = no rain in the 12 h window |
| `rain.from` | string `"HH:MM"` or `""` | Start of that rainy hour; `""` when `in_h` is `-1` |
| `rain.until` | string `"HH:MM"` or `""` | First dry hour after it; `""` if it rains to the end of the window (or no rain) |

The panel shows `rain` as `NO RAIN 12H` (`in_h` -1), `RAIN <from>` (`in_h`
> 0), `RAIN TILL <until>` (`in_h` 0 with `until`), `RAIN NEXT 12H`
(`in_h` 0 without `until`).

#### `indoor`

| Field | Type | Meaning |
|---|---|---|
| `temp_c` | number, 1 decimal | Room temperature, °C |
| `humidity_pct` | number, 1 decimal | Relative humidity, % (40-60 is comfortable indoors) |
| `pressure_hpa` | number, 1 decimal | Air pressure at the sensor, hPa (not sea-level corrected) |

#### `air`

| Field | Type | Meaning |
|---|---|---|
| `aqi` | int | European Air Quality Index (0 and up, lower is better) |
| `aqi_label` | string | Band of `aqi`: `GOOD` (≤ 20), `FAIR` (≤ 40), `MODERATE` (≤ 60), `POOR` (≤ 80), `VERY POOR` (≤ 100), `EXTREME` (> 100) |
| `pollen.alder`, `.birch`, `.grass`, `.mugwort`, `.ragweed` | int | Pollen in grains/m³, rounded. `0` = none or off season. The panel's rough scale: < 1 none, 1-10 low, 11-50 medium, > 50 high |

All five pollen keys are always present.

#### `warning`

`null` when unknown (no successful fetch, or the last one failed). Otherwise:

| Field | Type | Meaning |
|---|---|---|
| `count` | int | Warnings for Berlin in effect or announced; `0` = none. The other fields are **only present when `count` > 0** |
| `event` | string | The one warning worth showing, DWD's English label uppercased, e.g. `HEAVY RAIN`. Picked as: started beats upcoming, then more severe, then first listed |
| `severity` | string | `minor`, `moderate`, `severe`, `extreme` (DWD map colours yellow, orange, red, violet) |
| `started` | bool | Whether it is already in effect |
| `onset` | string | When it starts/started: local `"HH:MM"` if today, else `"DD/MM"` |

#### `next_holiday`

`null` until the holiday list is fetched, or when none is left this year
(the next year's list is fetched after 26 Dec).

| Field | Type | Meaning |
|---|---|---|
| `date` | string `"YYYY-MM-DD"` | Next Berlin public holiday, today included (it's today when `date` equals the top-level `date`) |
| `name` | string | English name, uppercased, letters/digits/spaces only: `GERMAN UNITY DAY`, `ST STEPHENS DAY` |

#### `bvg`

`null` until the first successful departures fetch. The source (the
community API `v6.bvg.transport.rest`) has long outages (503s), so **expect
`null` for long stretches**; a switch to the official VBB API is planned,
with the same fields here.

Departures are only fetched while the BVG screen is on the panel **or
`/api/status` was read in the last 2 minutes**. So a client polling the
status keeps them fresh; when nobody polls and the screen isn't up, the
list can be old (entries already gone are dropped, see `in_min`).

| Field | Type | Meaning |
|---|---|---|
| `walk_min` | int | Minutes it takes to walk to the stop: departures with `in_min` < `walk_min` can't be caught (the panel hides them) |
| `walk_comfort` | int | Minutes for a relaxed walk. The panel's hint for the first catchable train: `LEAVE IN <in_min - walk_comfort>` while positive, `GO NOW` at 0, `HURRY` below |
| `departures` | array | Up to 6, soonest first, one fixed stop and direction (configured on the device). Only departures still ahead (`in_min` ≥ 0); cancelled ones are already left out. Can be empty (nothing in the next hour) |
| `departures[].line` | string | e.g. `U5` |
| `departures[].direction` | string | Destination, uppercased ASCII (umlauts spelled out), e.g. `HAUPTBAHNHOF` |
| `departures[].time` | string `"HH:MM"` | Real departure time (delay included), local |
| `departures[].in_min` | int | Minutes from the response's `time` until then |

### `POST /api/screen?go=<value>`

| `go` | Effect |
|---|---|
| `next`, `prev` | Next / previous screen, wrapping (order: home, outdoor, air, indoor, bvg), like turning the knob. If the panel is off it only wakes it, without switching (same as the knob) |
| `home`, `outdoor`, `air`, `indoor`, `bvg` | Show that screen; also turns the panel on |

### `POST /api/panel?set=<value>`

| `set` | Effect |
|---|---|
| `on`, `off` | Panel on / off |
| `toggle` | Flip it, like pressing the knob |

### Command responses and timing

- Success: `200`, `application/json`, body `{"ok":true}`. The command is
  queued at that point, not yet applied.
- It's applied within a few hundred ms (measured: done 300 ms later). But
  while the device runs a network fetch in its main loop (at boot, and
  every 15 min for a few seconds) it waits until that ends; seen up to
  ~10 s right after boot.
- To show the new state, re-read `/api/status` shortly after (the
  display's own page waits 150 ms, then polls every 5 s). `screen` /
  `panel_on` change once it's applied.

### Errors

| Status | When | Body |
|---|---|---|
| `400` | Missing or unknown `go` / `set` value (values are lowercase, exact) | `text/html`, a short message |
| `404` | Unknown path | `text/html` |
| `405` | Wrong method (e.g. `GET /api/panel`) | `text/html` |
| `500` | Status JSON couldn't be built, or the command queue is full | `text/html` |

Network-level failures are normal too: the display reboots after a
firmware flash or a power blip (unreachable for ~15 s), and it's gone when
unplugged. Treat a timeout or refused connection as "display offline" and
keep showing the last data, marked stale.

## Client guidelines

- **Polling**: every 5 s while visible is fine (that's what the display's
  own page does); 30-60 s or longer for a background widget. Don't go
  below ~1 s. There is no push, WebSocket or long-poll.
- The server is small: requests are handled one at a time, about 7
  connections at most (least recently used ones get closed). Use short
  timeouts (~3-5 s) and don't fire requests in parallel.
- Everything is local: the device doesn't cache per client, and each
  `/api/status` builds a fresh response from what's in memory.
- **Unknown fields**: new fields may be added; ignore ones you don't know.
  A removed or renamed field changes `status.example.json` in this repo.
- No history is available from the device. The readings also go to the
  home InfluxDB/Grafana dashboard (see `server/README.md`), but that's
  separate and not part of this API.

## Trying it out

```sh
curl http://desk.local/api/status
curl -X POST "http://desk.local/api/screen?go=air"
curl -X POST "http://desk.local/api/panel?set=toggle"
avahi-browse -rt _http._tcp   # Linux: shows the "Desk display" service
```
