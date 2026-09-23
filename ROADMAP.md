# Roadmap

One task per session. Tick it off (with the date) when it's done and verified
on the physical display, and note anything the next task needs to know.

- [x] **1. Automatic summer/winter time** (2026-09-23): POSIX TZ rule
  `CET-1CEST,M3.5.0,M10.5.0/3` in `main/clock.c` replaces the hand-flipped
  `UTC_OFFSET_HOURS`. RTC still stores UTC.
- [ ] **2. Sunrise/sunset + wind/UV**: same Open-Meteo request, extra fields
  (`daily=sunrise,sunset,uv_index_max`, `current` wind speed). Where it goes on
  screen depends on task 3; until then, fits the free rows 6-7.
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
