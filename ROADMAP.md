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
- [ ] **4. BVG departures screen**: next departures for one stop via the BVG
  public API (`v6.bvg.transport.rest`, no key). Needs from the user: the stop,
  and which lines/directions matter.
- [ ] **5. Later, from the Freenove kit**
  - [ ] PIR motion sensor: screen on only when someone is at the desk (OLED
    burn-in protection). Powered from VIN (5V), output is 3.3V-safe.
  - [ ] Photoresistor: auto-dim in a dark room (night mode)
  - [ ] Passive buzzer: timer / "train leaves soon" alert

Other open items (not scheduled): WiFi connect timeout, battery backup, CO2
sensor (SCD41 was the pick if it happens) — see Open questions in `CLAUDE.md`.
