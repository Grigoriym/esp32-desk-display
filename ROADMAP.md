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
    on page 0 of all of them. Turn = next/previous (wraps), press = HOME.
    New screens (BVG, task 4): add a `screen_t` value and a `case`.
    Not done: auto-return to HOME after idle, a screen-position indicator.
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
- [ ] **4c. Code quality: lint, tests, CI** (Android analogues: ktlint,
  detekt/lint, unit tests, GitHub Actions)
  - [x] clang-format (2026-09-25): `.clang-format` + `tools/format.sh
    [--check]`, whole codebase reformatted in one commit.
  - [ ] Host unit tests (Unity/gcc on the PC): first split the JSON parsing
    out of `weather.c`/`bvg.c` into `parse_*(const char *json, ...)` and test
    against saved real responses; then `utc_to_epoch()`/DST dates, weather
    code -> icon, BVG leave/hurry logic, encoder decode, glyphs as ASCII art.
  - [ ] clang-tidy (via `idf.py clang-check`) or cppcheck; maybe `-Wextra`
    for `main/` only.
  - [ ] GitHub Actions: firmware build (espressif/idf image), format check,
    host tests.
- [ ] **5. Later, from the Freenove kit**
  - [ ] PIR motion sensor: screen on only when someone is at the desk (OLED
    burn-in protection). Powered from VIN (5V), output is 3.3V-safe.
  - [ ] Photoresistor: auto-dim in a dark room (night mode)
  - [ ] Passive buzzer: timer / "train leaves soon" alert

WiFi connect timeout: done 2026-09-25, see Open questions in `CLAUDE.md`.

Other open items (not scheduled): battery backup, CO2
sensor (SCD41 was the pick if it happens) — see Open questions in `CLAUDE.md`.
