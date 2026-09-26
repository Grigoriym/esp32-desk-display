# ESP32 Desk Display

A small always-on desk companion: an ESP32 drives a 0.96" OLED that shows
the time, the weather outside and inside, air quality and pollen, DWD
weather warnings, public holidays and the next U-Bahn departures. A rotary
knob switches between screens. Readings are also uploaded to a self-hosted
InfluxDB + Grafana dashboard, and a phone on the same WiFi can see them and
switch screens at `http://desk.local`.

Written in C on ESP-IDF, with a small hand-rolled SSD1306 driver and no
graphics library. All data sources are free and need no API key.

## Screens

Every screen has the time (`HH:MM`) flush left and the date (`DD/MM/YYYY`)
flush right on the top row. Turn the knob to go to the next or previous
screen (it wraps around). Press it to turn the panel off, and again to turn
it back on. Turning the knob while the panel is off also wakes it.

| Screen | Shows |
|---|---|
| **HOME** | DWD weather warning if one is out, otherwise the next public holiday; weather icon + outdoor temperature; indoor `IN 25C 43H`; rain hint (`RAIN 14:00`, `RAIN TILL 16:00`, `NO RAIN 12H`) |
| **OUTDOOR** | Sunrise/sunset, wind speed, today's max UV index |
| **AIR** | European AQI with its label, the two strongest pollen types and their levels |
| **INDOOR** | Temperature, humidity, pressure in hPa (BME280) |
| **BVG** | Next U-Bahn departures from one stop in one direction, with a `LEAVE IN N` / `GO NOW` / `HURRY` hint. One click counter-clockwise from HOME |

The font only has A-Z, 0-9, `:`, `-` and `/`, so `H` stands in for `%`.

At boot a status screen shows each subsystem coming up, with `--` changing
to `OK` or `NO`:

```
         HELLO

OLED    OK   PWR     OK
RTC     OK   BME     OK

WIFI    OK   NTP     OK
WEATHER OK
```

`PWR NO` means the last reset was a brownout (weak USB port or cable).

## Data sources

| Data | Source | Refresh |
|---|---|---|
| Time | NTP, kept through power loss by the DS3231 RTC (stores UTC) | RTC at boot, NTP after WiFi is up |
| Weather, sun, wind, UV, rain | [Open-Meteo](https://open-meteo.com/) forecast API | 15 min |
| Air quality, pollen | Open-Meteo air-quality API | 15 min |
| Weather warnings | DWD via [Bright Sky](https://brightsky.dev/) `/alerts` | 15 min |
| Public holidays | [Nager.Date](https://date.nager.at/) (Berlin + nationwide) | once a year |
| Departures | [`v6.bvg.transport.rest`](https://v6.bvg.transport.rest/) (community-run, has outages) | 60 s, only while the BVG screen is showing |
| Indoor | BME280 | 10 s |

A failed weather fetch is retried after 30 s, then at growing intervals up
to the normal 15 minutes.

## Resilience

- No RTC: the clock waits for NTP. No NTP: the RTC time is used.
- No WiFi within 15 s: boot shows `WIFI NO` and goes to the main screens on
  RTC time. WiFi keeps retrying in the background, and NTP and the weather
  follow once it connects.
- The BVG fetch and the dashboard upload run in their own tasks, so a slow
  or down server never freezes the clock or the knob.
- Summer/winter time switches automatically (POSIX TZ rule).

## Hardware

| Part | Notes |
|---|---|
| ESP32 DevKit (WROOM-32, 30-pin, 4 MB flash) | |
| 0.96" 128x64 I2C OLED, SSD1306/SSD1315 | I2C `0x3C` |
| DS3231 RTC module with coin cell | I2C `0x68`. Optional but recommended |
| BME280 | I2C `0x76`. Optional (indoor readings) |
| KY-040 rotary encoder | GPIO25/26/27. Optional (screen switching) |

All I2C modules share one bus (SDA GPIO21, SCL GPIO22), each wired
**directly to the ESP32, in parallel**, and powered from **3V3, never
5V**. Full pin table: [`docs/WIRING.md`](docs/WIRING.md).

## Build and flash

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) (developed
on master, v6.x).

```sh
cp main/wifi_secrets.h.example    main/wifi_secrets.h     # SSID/password
cp main/bvg_secrets.h.example     main/bvg_secrets.h      # stop + direction IDs
cp main/metrics_secrets.h.example main/metrics_secrets.h  # dashboard host/token, empty = off
. ~/esp/esp-idf/export.sh
idf.py -p /dev/ttyUSB0 build flash monitor
```

The `*_secrets.h` files are gitignored, so credentials never get committed.
cJSON is pulled in automatically by the component manager.

## Dashboard

`server/` holds an InfluxDB 2 + Grafana setup in Docker Compose. The display
POSTs indoor, outdoor, air-quality and device values (heap, WiFi signal,
uptime) every minute. Run it on an always-on machine on the display's LAN:
see [`server/README.md`](server/README.md) and
[`server/INSTALL.md`](server/INSTALL.md).

## Phone access and API

The display serves a small page at **`http://desk.local`** (mDNS; the IP
from the boot log works too) with everything the screens show, refreshed
every 5 s, plus buttons for the screens and the panel. Home WiFi only, and
**no authentication**: anyone on the LAN can read it and switch screens.

| Request | Does |
|---|---|
| `GET /api/status` | JSON: `time`, `date` (`YYYY-MM-DD`), `screen`, `panel_on`, and `outdoor`, `indoor`, `air`, `warning`, `next_holiday`, `bvg`. A section is `null` until its first fetch or read succeeds (`--` on the panel); `warning` is `{"count": 0}` when there is none |
| `POST /api/screen?go=next\|prev\|home\|outdoor\|air\|indoor\|bvg` | Switch screens, like turning the knob. A screen by name also turns the panel on; `next`/`prev` while it's off only wake it, as the knob does |
| `POST /api/panel?set=on\|off\|toggle` | Panel on/off, like pressing the knob |

Commands answer `{"ok":true}` (`400` for a bad value) and apply within a
few hundred ms, or once a running fetch finishes. While `/api/status` is
being read (the last 2 minutes), departures are fetched even if the BVG
screen isn't up. The server also announces itself as `_http._tcp` for
discovery (Android NSD). Full contract (every field, discovery, errors,
polling): [`docs/API.md`](docs/API.md).

## Configuration

Settings are hardcoded constants. Change them and reflash:

| What | Where |
|---|---|
| Location (default: central Berlin) | `WEATHER_URL`, `AIR_URL`, `ALERTS_URL` in `main/weather.c` |
| Holiday region (default: `DE-BE`) | `HOLIDAYS_REGION` in `main/holidays.c` |
| Timezone (default: Berlin, automatic DST) | `LOCAL_TZ` in `main/clock.c` |
| Refresh intervals, WiFi timeout | `*_SECONDS` in `main/main.c` |
| BVG stop, direction, walking time | `main/bvg_secrets.h` |
| Display orientation | `0xA0`/`0xC0` in the init sequence in `main/display.c` (this build's panel is mounted upside-down; use `0xA1`/`0xC8` for the default orientation) |

## Development

Run these before pushing; CI (`.github/workflows/ci.yml`) runs the same on
every push:

| Script | Does |
|---|---|
| `tools/test.sh` | Host unit tests (Unity, ASan/UBSan) for the pure logic: parsers, screen layouts, font and icon bitmaps, time math, encoder decoding |
| `tools/format.sh` | clang-format (`--check` only reports) |
| `tools/lint.sh` | clang-tidy, checks in `.clang-tidy` |
| `tools/size_check.sh` | Fails if the app partition has under 15% free |
| `tools/serial_log.py [seconds] [regex]` | Resets the board and captures/greps the serial log |

clang-format and clang-tidy come from ESP-IDF's optional `esp-clang` tool.
New files are only checked once they're `git add`-ed.

## Project layout

```
main/
  main.c            boot sequence, status screen, main loop, knob handling
  screens.c         what each screen shows (pure, host-tested)
  display.c         SSD1306 driver; font.c: 5x7 font and icons
  clock.c           NTP sync, DS3231; clock_time.c: time math
  wifi.c            WiFi station
  http.c            shared HTTPS GET
  weather.c         weather, air quality and warnings fetch
  weather_parse.c   \
  air_parse.c        | JSON parsing (pure, host-tested)
  alerts_parse.c    /
  holidays.c        public holidays; holidays_parse.c
  bvg.c             departures, own task; bvg_parse.c
  bme280.c          indoor sensor driver
  encoder.c         KY-040 ISR + button task; encoder_decode.c
  metrics.c         dashboard upload, own task; metrics_format.c
  health.c          periodic heap/stack report in the log
  web.c             phone page + JSON API, mDNS; web_api.c (pure,
                    host-tested), page in web_index.html
test/               host unit tests + fixtures
tools/              test, format, lint, size and serial scripts
server/             InfluxDB + Grafana (Docker Compose)
docs/WIRING.md      pin table
```

Hardware bring-up tests (I2C scan, OLED pattern, BME280 chip ID, encoder)
live in a separate sibling project, `esp32-hw-checks`.

## License

[Apache License 2.0](LICENSE)
