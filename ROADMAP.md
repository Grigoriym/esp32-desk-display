# Roadmap

One task per session. Tick it off (with the date) when it's done and verified
on the physical display, and note anything the next task needs to know.

- [x] **1. Automatic summer/winter time** (2026-09-23): POSIX TZ rule
  `CET-1CEST,M3.5.0,M10.5.0/3` in `main/clock.c` replaces the hand-flipped
  `UTC_OFFSET_HOURS`. RTC still stores UTC.
- [x] **2. Sunrise/sunset + wind/UV** (2026-09-23): same Open-Meteo request
  plus `daily=sunrise,sunset,uv_index_max&timezone=Europe/Berlin` (sun times
  arrive already local, DST included). `weather_fetch()` now fills a
  `weather_t`; `draw_weather()` in `main/main.c` draws rows 3, 6
  (`RISE 06:53` / `SET 19:02`) and 7 (`WIND 6KMH` / `UV 4`). UV is today's
  max, not current. Rows 6-7 are candidates to move to their own screen in
  task 3.
- [x] **3. KY-040 rotary encoder + multiple screens** (2026-09-24)
  - [x] Bring-up check in `../esp32-hw-checks` (2026-09-24): CLK D25, DT
    D26, SW D27 (see `docs/WIRING.md`), connected through a socket on the
    perfboard. ISR quadrature decode: **4 steps per detent** confirmed (full-step
    part), direction correct, no missed or reversed clicks on a fast spin
    (~30-40 ms per detent), button clean with 30 ms debounce, no double
    presses. The decode + debounce code there can be lifted into this firmware.
  - [x] Screen rotation (2026-09-24): `main/encoder.c` (ISR rotation + polled
    button -> event queue). Screens in `main/main.c` (`screen_t`,
    `draw_screen()`): HOME (icon+outdoor temp, `IN 25C 43H`), OUTDOOR
    (sunrise/sunset, wind/UV), INDOOR (temp, humidity, pressure). Clock row
    on page 0 of all of them. Turn = next/previous (wraps), press = HOME
    (press = panel off/on since 2026-09-25, verified on the panel).
    New screens (BVG, task 4): add a `screen_t` value and a `case`.
    Auto-return to HOME after idle and a screen-position indicator were
    considered and dropped (2026-09-25): not wanted.
- [x] **4. BVG departures screen** (2026-09-24; verified on the panel with
  real data during one of the wrapper's rare successful fetches: only U5
  towards Hbf, minutes match the times, nothing under 6 min). U Cottbusser Platz, U5 towards Hbf; stop
  and direction IDs in gitignored `main/bvg_secrets.h` (template:
  `bvg_secrets.h.example`). `main/bvg.c` fetches in a background task, only
  while the screen is showing, every 60s; the API's `direction=<next stop
  id>` filter picks the direction (U Kienberg = towards Hbf), which also
  catches short-turning trains. Screen 4 (one click CCW from HOME): title
  `U5 HAUPTBAHNHOF`, `LEAVE IN N` (12+ min left), `GO NOW` (11), `HURRY` (6-10), then
  up to 3 trains >= 6 min away as `HH:MM   N MIN`. Verified: no crash, and a
  down API no longer freezes clock/encoder.
- [ ] **4b. Switch BVG to the official VBB API** — *blocked on the key*.
  Why: the community wrapper
  `v6.bvg.transport.rest` returns 503 on every data request and has done so
  mostly since July ([bvg-rest#30](https://github.com/derhuerst/bvg-rest/issues/30),
  no maintainer reply), while BVG's own backend (`bvg.hafas.cloud`, what the
  wrapper relays) answered fine when queried directly, so the wrapper is
  what's broken. Decision: switch to the **official VBB API**. Access
  requested by email to api@vbb.de on 2026-09-24 (test system first, then
  production after accepting their terms). When the key arrives: key into
  `bvg_secrets.h`, new parser in `bvg.c` (different response format;
  screen and background task stay). Talking to `bvg.hafas.cloud` directly
  was considered and not chosen: unofficial web-app backend with a token
  copied from BVG's web app. Self-hosting the wrapper (Docker image
  `derhuerst/bvg-rest:6`, last updated Oct 2025, untested) was also
  considered as a bridge and not chosen: same unofficial backend, and the
  ESP32 can't join Tailscale, so it would need a host on the display's
  own LAN or a public HTTPS one. It would need only a URL change in
  `bvg.c` (same response format) if revisited.
- [x] **4c. Code quality: lint, tests, CI** (2026-09-25) (Android analogues: ktlint,
  detekt/lint, unit tests, GitHub Actions)
  - [x] clang-format (2026-09-25): `.clang-format` + `tools/format.sh
    [--check]`, whole codebase reformatted in one commit.
  - [x] Host unit tests (2026-09-25): `tools/test.sh`, Unity + gcc with
    ASan/UBSan. JSON parsing split into `weather_parse.c` / `bvg_parse.c`;
    12 tests (parsers, weather code -> icon, stop-name cleanup).
  - [x] More host tests (2026-09-25): pure logic split out again, like
    the parsers: `clock_time.c` (`utc_to_epoch()` vs glibc `timegm()` for
    every day 2000-2100, BCD, the Berlin DST switch moments through
    `LOCAL_TZ`), `encoder_decode.c` (quadrature decode + button debounce,
    fed by the ISR / button task), `font.c` (glyph tables + `font_blit()`).
    Glyphs and weather icons are checked as ASCII art (`test/art.h`): a
    wrong bit prints the picture as drawn next to the expected one. 47
    tests. Real BVG capture added as `bvg_real.json` (2026-09-25).
  - [x] clang-tidy (2026-09-25): `.clang-tidy` + `tools/lint.sh`, clean.
    Tidied on the way (none were live bugs): int->time_t widening in `utc_to_epoch()` made
    explicit, int->float conversions in `bme280.c` made explicit, missing
    `default:` in the umlaut switch.
  - [x] Split `app_main()` and `draw_screen()` (2026-09-25): screen text in
    `screens.c` (pure, 8 host tests incl. the LEAVE IN / GO NOW / HURRY
    boundaries and the midnight wrap); `app_main()` is now boot steps +
    one `tick_*()` per periodic job. Complexity NOLINTs gone.
  - [x] GitHub Actions (2026-09-25): `.github/workflows/ci.yml`, firmware
    build + format check + host tests + clang-tidy.
- [x] **5. Later, from the Freenove kit** (2026-09-25)
  - PIR motion sensor and passive buzzer: dropped (2026-09-25), not wanted
    (display runs ~1 h/day, so no burn-in concern; no sounds).
  - [x] Photoresistor (2026-09-25): LDR + 10k divider soldered on D34
    (`docs/WIRING.md`), reads fine (`esp32-hw-checks` LDR check). **Auto-dim
    dropped**: this SSD1315 can't visibly dim. Tried with a knob-driven
    trial: contrast 0x01-0x40 look identical, 0x00 switches the panel off
    (also with shorter pre-charge / lower VCOMH), 0xFF only slightly
    brighter. The dimming firmware (ADC reader, hysteresis logic) was
    written and removed unmerged. **LDR removed from the board**
    (2026-09-25): no use for it without dimming.

- [x] **6. Rain hint on HOME** (2026-09-25, `NO RAIN 12H` seen on the
  panel): page 7 shows `NO RAIN 12H`, `RAIN 16:00` (next rainy hour),
  `RAIN TILL 15:00` (raining now) or `RAIN NEXT 12H`. Same Open-Meteo
  request plus `hourly=precipitation_probability&forecast_hours=12`
  (hourly starts at the current hour, local time); an hour counts as rainy
  at >= 50% (`WEATHER_RAIN_MIN_PROB`), null = dry. Parsing in
  `weather_parse.c`, text in `screens.c`, both host-tested. Response is
  ~1.2 KB of the 2 KB buffer in `weather.c`.

- [x] **7. Air quality + pollen screen** (2026-09-25, `AQI 23 FAIR` /
  `POLLEN NONE` seen on the panel): screen AIR, between OUTDOOR and INDOOR (two clicks CW from
  HOME). `AQI 23` / `FAIR` (European AQI bands: GOOD <=20, FAIR <=40,
  MODERATE <=60, POOR <=80, VERY POOR <=100, EXTREME), then the two
  strongest of alder/birch/grass/mugwort/ragweed as `BIRCH` / `HIGH`, or
  `POLLEN NONE`. Pollen scale is one rough scale for all types (grains/m3:
  1-10 LOW, 11-50 MED, >50 HIGH). Current values from Open-Meteo's
  air-quality API, fetched with the weather; an air failure makes the
  fetch retry with the weather backoff. Pollen is 0 off season (Sep-Jan),
  so the pollen rows only get real data from spring.

- [x] **8. Dashboard** (2026-09-25): the display POSTs its readings every
  minute to InfluxDB, Grafana shows them (`server/`, Docker Compose, setup
  in `server/README.md` / `server/INSTALL.md`). First tested with the stack
  on the dev machine; since 2026-09-25 it runs on the always-on home box,
  Grafana at `http://192.168.0.139:34897/`, firmware reflashed with its IP
  and token (`upload OK` in the boot log), dev-machine stack and volumes
  removed.

- [x] **9. DWD weather warnings on HOME** (2026-09-25; the fetch runs on
  the device, `alerts fetched (none)`, but Berlin had no warning to see on
  the panel yet): Bright Sky
  `https://api.brightsky.dev/alerts?lat=52.52&lon=13.405&tz=Europe%2FBerlin`
  (free, no key; warn cell 711000101 "Berl. - Mitte"), fetched after the
  air quality every 15 min. While a warning is out, HOME shows it on
  page 1, under the clock (the rain hint stays on page 7): DWD's English label uppercased (`HEAVY RAIN`),
  or for one that hasn't started yet the label plus its start, `18:00` today
  or `26/09` later. Pick: started beats upcoming, then severity
  (minor/moderate/severe/extreme = DWD yellow/orange/red/violet). Test
  messages and all-clears are skipped. Heap low point 139 KB after it
  (was 145). Still open: see a real warning on the panel, and replace the
  hand-written fixture with a capture of it.

- [x] **10. Berlin public holidays** (2026-09-25; `NEXT HOLIDAY 03/10` seen
  on the panel; the fetch runs on the
  device, `holidays 2026 fetched (10 in Berlin, next 20261003 GERMAN UNITY
  DAY)`): Nager.Date `https://date.nager.at/api/v3/PublicHolidays/<year>/DE`
  (free, no key), keeping `global: true` or `DE-BE` in `counties`. Fetched
  at the first chance after boot (clock set + WiFi), then only when the
  next year's list is needed (after 26 Dec); a failure retries in 30 min.
  HOME page 1 (under the clock) shows `NEXT HOLIDAY` + `DD/MM`, or the
  holiday's English name on the day (`GERMAN UNITY DAY`, `.`/`'` dropped:
  `ST STEPHENS DAY`); a DWD warning takes that row. Each holiday is one day
  in the API, so back-to-back ones (25/26 Dec) just change the line.
  School holidays (multi-day) are a different source, not covered. Needed
  `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_CROSS_SIGNED_VERIFY` (see CLAUDE.md).

- [x] **11. Web page + JSON API on the display** (2026-09-26; API checked
  from the PC over `desk.local`, phone page and panel switching confirmed
  by the user; see README "Phone access and API") (phone access, home WiFi
  only): `esp_http_server` + mDNS so it answers at `http://desk.local`.
  `GET /api/status` = the current `screen_data_t` as JSON (time, outdoor/
  indoor, AQI/pollen, BVG, warning/holiday); `POST /api/panel` (off/on) and
  `/api/screen` (next/prev/by name), same effect as the knob, handed to the
  main loop through the encoder event queue rather than touching the display
  from the server task. A tiny HTML page at `/` for any phone browser. JSON
  building pure + host-tested (like `metrics_format.c`); the server task
  goes in `TASKS[]` in `health.c`. Budget: ~30-50 KB flash (35% free), check
  heap after. No auth (LAN only), note it in the README.
  Done as planned, plus: web commands reuse the encoder queue with a new
  `input_event_t` (`main/input.h`); reading `/api/status` keeps BVG fetching
  for 2 min (else `bvg` would be stale off-screen). Cost: +74 KB flash (30%
  free, was 35%; mDNS is most of it), heap lowest 116 KB (was 139), stack
  left httpd 2.4 KB / mdns 2.2 KB. A command sent during a boot fetch waits
  until the fetch ends (seen: ~10 s).

- [ ] **12. Android app** (Kotlin/Compose, separate repo, built by a
  separate agent from the user's app template; everything it needs about
  the device is in `docs/API.md` + `docs/api/status.example.json`): finds
  the display via NSD (`_http._tcp`, mDNS name from 11), reads
  `/api/status`, one card per screen + panel/screen buttons, and a Glance
  home-screen widget (outdoor/indoor temp, next departure).

WiFi connect timeout: done 2026-09-25, see Open questions in `CLAUDE.md`.

Other open items (not scheduled): battery backup, CO2
sensor (SCD41 was the pick if it happens) — see Open questions in `CLAUDE.md`.
