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
- [ ] **3. KY-040 rotary encoder + multiple screens**
  - [ ] Bring-up check in `../esp32-hw-checks` first (rotation direction,
    button press, debounce), pick free GPIOs
  - [ ] Screen rotation in this firmware: encoder turns between screens
    (e.g. clock/weather, sun/wind/UV, indoor, BVG)
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
