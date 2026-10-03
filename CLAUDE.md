# ESP32 Desk Info Display

## Concept
A WiFi-connected desk companion on a small OLED screen: live clock (NTP) + current
weather, always on. First "real" project after the esp32 lessons repo — not a
tutorial exercise, meant to actually sit on the desk.

## Hardware
**Part facts live in the shared sheets, `../grappim-watcher/docs/esp32/parts/`**
(one file per part: pinout and pin order, voltage, I2C address, current draw,
measured dimensions, mounting, quirks, hw-checks status; index and inventory in
its `README.md`). **A new fact about a part goes in its sheet there, not here.**
This file holds only what is specific to this build. Generic rules:
`../grappim-watcher/docs/esp32/WIRING_RULES.md`, `ENCLOSURE_PLAYBOOK.md`,
`FIRMWARE_PLAYBOOK.md` (same folder).

- **Board**: ESP32 DevKit 30-pin (`esp32-devkit-30pin.md`), flashes on
  `/dev/ttyUSB0`. The blue GPIO2 LED is off in this firmware.
- **Modules in this build**, all I2C ones in parallel on D21 (SDA) / D22 (SCL),
  all on 3V3:
  | Part | Sheet | Here |
  |---|---|---|
  | OLED SSD1315 | `oled-ssd1315.md` | 0x3C; mounted upside-down, fixed in software (see Display driver notes) |
  | DS3231 RTC | `ds3231.md` | 0x68 (+0x57, 0x5F); boot time source since 2026-09-23 |
  | BME280 | `bme280.md` | 0x76; read since 2026-09-23 (`main/bme280.c`) |
  | SCD41 CO2 | `scd41.md` | 0x62; `main/scd41.c` since 2026-09-26 |
  | KY-040 | `ky-040.md` | D25/D26/D27; switches screens since 2026-09-24 |
- **Pin table for every module lives in `docs/WIRING.md`**; keep it updated
  when wiring changes. **Current state (2026-09-26): everything is on a
  solderless plastic breadboard.** A first all-soldered perfboard attempt
  (modules in female sockets, ESP32 plugged down into sockets) was abandoned
  and moved back to the breadboard. The final soldered board is designed
  together with the enclosure (ROADMAP task 15): a carrier board sized to
  the case, keyed connectors out to each module.
- **Parts history**: ordered 2026-09-17 (sourced away from AliExpress,
  brand-name sellers, to avoid EU import duty; exact per-item source not
  logged), arrived and tested 2026-09-19 via `esp32-hw-checks`: no DOA units,
  no substitutions. Milestones 1-4 were built on this hardware.
- **Not part of this build**: ESP32-WROVER-B DevKit (not ordered; the Freenove
  kit's WROVER is held in reserve for a future colour-TFT/LVGL project),
  MQ-135 (air quality) and RC522 (RFID) discussed as optional future adds, a
  2.4"-3.5" ILI9341/ILI9488 touch TFT as a future colour version.

## Power & bus budget (guardrail — check before wiring any new module)
Per-part current draw is in each part sheet; the rules (3V3 regulator and USB
cap ~500 mA, keep the summed peak under ~450 mA, I2C pull-ups in parallel, 3V3
not 5V, 5V parts on VIN, battery runtime) are in
`../grappim-watcher/docs/esp32/WIRING_RULES.md`.

**This build's total** (ESP32 + WiFi, OLED, DS3231, BME280, KY-040, SCD41):
**~135-175 mA typical, ~585 mA peak**. The peak is the worst case only if an
SCD41 pulse (~205 mA, short) lands on a WiFi TX burst: brief; watch for
`PWR NO` / brownout. No extra bulk cap is fitted for the SCD41.

Here:
- New module → add its draw to the total above, re-check the peak.
- Four I2C modules are on the bus, each with its own pull-ups (limit: ~5-6).
- New I2C address → add it to `KNOWN_I2C` in `main/main.c` (boot log warns on
  any unknown address).
- Runtime check: status screen `PWR NO` / log `last reset was a BROWNOUT`
  means the supply sagged.

## Software / ESP-IDF components used
| Need | Component |
|---|---|
| I2C bus | `esp_driver_i2c` |
| BME280 | hand-rolled driver (`main/bme280.c`): chip-ID check, factory calibration, Bosch datasheet integer compensation, forced mode x1 oversampling |
| SCD41 (CO2) | hand-rolled `main/scd41.c`: stop, then **low-power periodic mode** (one reading per 30 s, ~3 mA); the main loop polls data-ready every 5 s (`tick_co2()`), first reading ~20-30 s after boot. CRC + conversion pure in `main/scd41_parse.c` (host-tested against the datasheet's examples). Its own temp reads a few °C high (self-heating), so indoor temp stays the BME280's. Shown as `CO2 812` on INDOOR p6 right, `CO2 OK/NO` boot cell. **Self-calibration (ASC) left on, factory defaults** (decided 2026-09-26): boot log `scd41: ASC enabled 1, target 400 ppm, initial period 44 h, standard period 156 h`. ASC only counts unbroken stretches of >= 4 h of measuring and assumes the lowest CO2 of each period is fresh air; the display runs overnight (counts) and the window is usually open at the desk (the fresh-air anchor). Nothing is written to the sensor's EEPROM (`persist_settings`, 2000-write limit, never on boot). If readings ever look low (e.g. a winter with little airing), turn ASC off + one-off FRC outdoors (0x362f, ~420 ppm, after 3 min running) |
| KY-040 encoder | hand-rolled `main/encoder.c`: rotation decoded in a GPIO any-edge ISR (4 steps/detent), button polled + debounced in its own task, both pushed to a FreeRTOS queue read by the main loop. The decode/debounce logic itself is pure, in `main/encoder_decode.c` (host-tested) |
| OLED framebuffer + text rendering | hand-rolled minimal SSD1306 driver (`main/display.c`) — decided against a component-manager package |
| WiFi station | `esp_wifi`, `esp_netif`, `nvs_flash` |
| Clock sync | SNTP (`esp_netif_sntp`) |
| Weather fetch | `esp_http_client` + `mbedtls` (`esp_crt_bundle_attach` for TLS, Open-Meteo is HTTPS-only) |
| Air quality + pollen | Open-Meteo air-quality API (free, no key), fetched right after the weather on the same 15-min cadence, same `http_get()` in `main/weather.c`; parsed in `main/air_parse.c` |
| DWD weather warnings | Bright Sky `/alerts` (free, no key, `tz=Europe/Berlin`), fetched after the air quality on the same cadence via `http_get()` into a 16 KB heap buffer (each warning is ~1 KB of German + English text); parsed in `main/alerts_parse.c`. Shown on HOME page 1, under the clock. A failed fetch blanks it rather than keep a stale one |
| Public holidays | Nager.Date (free, no key), `main/holidays.c` + pure `main/holidays_parse.c`: Berlin's (`DE-BE` + nationwide), fetched once a year (and the next year's after 26 Dec), shown on HOME page 1 unless a DWD warning is out. Its TLS chain is GTS Root R4 cross-signed by the old GlobalSign Root CA (not in the bundle), so `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_CROSS_SIGNED_VERIFY=y` (`sdkconfig.defaults`, ~700 B heap); symptom without it: `No matching trusted root certificate found` / `ESP_ERR_HTTP_CONNECT`. A new HTTPS host failing that way → check its chain with `openssl s_client -showcerts` |
| HTTP GET | `main/http.c` `http_get(url, buf, size)`, shared by weather/air/alerts/holidays; big responses (alerts 16 KB, holidays 8 KB) get a heap buffer for the fetch only |
| BVG departures | `esp_http_client` against `v6.bvg.transport.rest` (community-run, no key, **has outages**: 503 after 10s or no answer, the `v6.vbb` mirror too, seen 2026-09-24), in its own FreeRTOS task (`main/bvg.c`) so a slow/down API never blocks the main loop |
| Dashboard upload | `esp_http_client` plain-HTTP POST of InfluxDB line protocol every 60 s, in its own task (`main/metrics.c`, 4 KB stack) so a down server never blocks the main loop; lines built by the pure `main/metrics_format.c` (host-tested). Server side (InfluxDB 2 + Grafana, Docker Compose) in `server/`, see `server/README.md`; runs on the always-on home box, Grafana at `http://192.168.0.139:34897/` (since 2026-09-25, the dev-machine copy is gone). `METRICS_TOKEN` must be the `INFLUX_TOKEN` of *that* server's `server/.env`: a token from another install gets `401` |
| Phone page + JSON API | `esp_http_server` + mDNS (`espressif/mdns`, component manager) in `main/web.c`: `http://desk.local`, page embedded from `main/web_index.html` (`EMBED_TXTFILES`), JSON built by the pure `main/web_api.c` (host-tested). Commands go to the main loop through the encoder queue (`input_event_t`, `main/input.h`). Contract for clients in `docs/API.md` + `docs/api/status.example.json` (a host test fails if the firmware's JSON drifts from that file: **API change → update both**). No auth, LAN only. Since 2026-09-26 (ROADMAP 11); the Android app (ROADMAP 12) is a separate repo |
| JSON parsing | `cJSON` — **not bundled** in this ESP-IDF version (v6.1-dev); pulled via the component manager (`main/idf_component.yml` → `espressif/cjson`), lands in gitignored `managed_components/` |

## Display driver notes (`main/display.c`)
- Letters A-Z (added 2026-09-23 for the boot status screen) are the classic
  Adafruit GFX glcdfont 5x7 bitmaps, not hand-derived — all rendered correctly
  first time. `C` deliberately kept as the original open-sided variant. Anything
  outside A-Z/0-9/`:`/`-`/`/` (incl. space) renders blank. `/` is also
  glcdfont (added 2026-09-23 for the date).
- Digits and weather icons are **hand-derived bitmaps**. Found one real
  transcription bug on hardware (digit '9' had a missing mid-row bit). Since
  2026-09-25 they're checked on the host as ASCII art (`test/test_font.c`,
  `test_icon_art` in `test/test_weather_parse.c`, helper `test/art.h`):
  **new glyph/icon → draw the expected picture in the test first**, run
  `tools/test.sh`, and a wrong bit prints drawn vs expected with `<--` on
  the bad row. Still eyeball it on the panel after flashing.
- The font (glyph tables, `font_blit()`, `font_text_width()`) lives in the
  pure `main/font.c` since 2026-09-25; all three `display_draw_*` text
  functions go through `font_blit()`, which also drops glyphs past either
  edge (the old centered-text loop wrote out of bounds on too-long text).
- Boot grid cells are `"%-8s%s"` = 10 chars (59px) per column, which exactly fills
  128px with the two columns flush left/right. A status name longer than 8 chars
  or a status longer than 2 breaks the alignment/overlaps — keep names ≤ 8.
- **Main task stack is 8 KB** (`sdkconfig.defaults`, since 2026-09-24; also
  set in the gitignored `sdkconfig`). The 3584 default was just enough for
  the weather fetch and overflowed on the first BVG fetch (TLS + bigger
  JSON): symptom was a reboot the moment the screen was opened, log
  `A stack overflow in task main has been detected`. Any task doing an
  HTTPS fetch + cJSON parse needs ~8 KB. Crash captures: include
  `overflow|Guru|Backtrace|rst:` in the `serial_log.py` regex, or the panic
  gets filtered out.
- **Flash layout** (since 2026-09-25, in `sdkconfig.defaults`): 4 MB flash
  (`CONFIG_ESPTOOLPY_FLASHSIZE_4MB`, the IDF default assumed 2 MB) and the
  large single-app partition table: 1.5 MB app, ~35% free. Before that the
  app was in a 1 MB partition with 4% left. Almost all of the ~980 KB is
  ESP-IDF (WiFi ~370 KB, TLS/crypto ~200 KB, lwIP ~100 KB); this project's
  own code is ~10 KB. `sdkconfig.defaults` only applies when `sdkconfig`
  is regenerated: after changing it, delete the local `sdkconfig` and build.
  **`tools/size_check.sh`** (in CI after the build) fails below 15% free
  (`MIN_FREE_PCT`), well before the build's own hard stop at 100%.
- **RAM guardrail** (`main/health.c`, since 2026-09-25): 60 s after boot
  and every 5 min, logs free heap (now / lowest since boot / largest block)
  and each task's worst-case stack headroom (`health:` tag), with a warning
  under 40 KB heap or 1 KB stack. Baseline 2026-09-25 after a weather + BVG
  fetch: heap lowest 145 KB; stack left main 4.8 KB (of 8), bvg 4.4 KB (of
  8), enc_button 1.6 KB (of 2), sys_evt 1.7 KB; metrics 2.5 KB (of 4, after
  uploads, 2026-09-25). A new task goes in `TASKS[]` there. The 60 s report
  can come **before a task's first real run**, so its number is meaningless
  there: to see bvg's, open the BVG screen before it; metrics' first upload
  is at ~69 s, so read its number from the 5-min report (a `serial_log.py`
  capture of ~390 s, since the script resets the board). The "60 s" is
  60 main-loop ticks, and boot fetches that block the loop push it back:
  since the alerts + holidays fetches (2026-09-25) it lands at **~75 s**, so
  a 70 s capture misses it; use 90 s. After those two fetches: heap lowest
  139 KB, stack left main 4.5 KB. After the web API (2026-09-26, with
  requests served during the capture): heap lowest 116 KB, stack left
  httpd 2.4 KB (of 4), mdns 2.2 KB, main 4.2 KB, bvg 4.2 KB.
- `snprintf` into a buffer that can't hold the worst case fails the build
  (`-Werror=format-truncation`): size buffers for the longest possible
  value, not the typical one.
- **`CONFIG_FREERTOS_HZ=100`** here and in `esp32-hw-checks` (10 ms tick, see
  the firmware playbook): why the encoder is decoded in an ISR.
- **Pure logic is checked on the host before flashing** by `tools/test.sh`
  (see Host unit tests), incl. `utc_to_epoch()` and the TZ rule's switch
  dates (`test/test_clock_time.c`, against glibc: it checks the rule string,
  not picolibc's parser of it).
- **Brightness**: the panel can't be dimmed, only on/off (tests in
  `oled-ssd1315.md`). Contrast is 0x40 (was 0xCF, no visible difference).
- Panel orientation: `0xA0`/`0xC0` in the init sequence, flipped 180° from
  the SSD1306 default to match how the OLED is mounted. New mounting → flip
  these two bytes, not the wiring.

## Data source decisions (decided)
- **Weather API**: Open-Meteo (free, no signup/API key), HTTPS.
- **Location**: hardcoded, central Berlin (lat 52.52, long 13.405). No GPS.
- **RTC (DS3231)**: stores **UTC**, so the DST flip below never touches it. At boot
  `clock_rtc_init()` sets system time from it (skipped if absent or its OSF flag
  says the time is invalid); after every successful NTP sync the firmware writes
  the fresh time back. Boot log line `system time set from RTC` / `RTC not used`
  is the quickest way to check whether a swapped-in module works.
- **Timezone**: POSIX TZ rule `LOCAL_TZ` = `CET-1CEST,M3.5.0,M10.5.0/3` in
  `main/clock.c` (since 2026-09-23), so the Berlin DST switch is automatic —
  replaced the old hand-flipped `UTC_OFFSET_HOURS`. Because TZ is now set,
  `mktime()` means *local* time: RTC→epoch conversion uses the TZ-independent
  `utc_to_epoch()` helper instead (verified against glibc `timegm` for
  2000-2100, and the 2026-10-25 / 2027-03-28 switch moments, on the host).
- **BVG departures**: stop/direction IDs in gitignored `main/bvg_secrets.h`.
  Direction is filtered with the API's `direction=<next stop id>` (not the
  destination name, which misses short-turning trains; the maintainer's
  caveat about trusting it is bvg-rest#29). Source is the community wrapper
  for now, switching to the official VBB API once the key arrives (ROADMAP
  4b). **To tell a wrapper outage from a BVG one**, query BVG's backend
  directly: endpoint + auth in hafas-client's `p/bvg/base.js`
  (`bvg.hafas.cloud/apps/gate`, `StationBoard` request); on 2026-09-24 it
  answered in 0.1s while the wrapper 503'd. The wrapper's homepage
  answering 200 proves nothing: only data requests hit the upstream.
- **Weather refresh cadence**: every 15 minutes (`WEATHER_REFRESH_SECONDS` in
  `main/main.c`), decided 2026-09-19 — gentle on the API, fresh enough for a desk
  display.

## Secrets
Decided: plain gitignored header, `main/wifi_secrets.h` (real credentials, never
committed) with `main/wifi_secrets.h.example` as the committed template. Same
for `bvg_secrets.h` and `metrics_secrets.h` (dashboard host + InfluxDB token;
empty host = upload off), and `server/.env` for the server side. A new
`*_secrets.h` also needs its `cp ... .example` line in `.github/workflows/ci.yml`. `.gitignore`
already excludes `*_secrets.h` and `sdkconfig`.

## Build / flash
Flashing and checking a build on the board: use the `esp32-flash-verify` skill
(generic routine); this section keeps only what's specific to this repo.
`. ~/esp/esp-idf/export.sh && idf.py -p /dev/ttyUSB0 build flash` (same for
`../esp32-hw-checks`; remember to flash this project back after a swap).
Flash traps (transient errors, **checking the flash actually happened** with a
case-insensitive `-iE "error|failed|Done"` filter, which build is running):
`../grappim-watcher/docs/esp32/FIRMWARE_PLAYBOOK.md`.

## Code style (since 2026-09-25)
`tools/format.sh` formats all tracked C sources with clang-format (style in
`.clang-format`, CLion picks it up too); `tools/format.sh --check` only
reports and fails, for CI. Run it before committing. clang-format (and
clang-tidy) come from ESP-IDF's optional `esp-clang` tool, installed with
`python $IDF_PATH/tools/idf_tools.py install esp-clang`. Hand-aligned
tables that clang-format would flatten go between
`// clang-format off` / `// clang-format on` (see `weather_icon_for_code()`).
`format.sh`, `lint.sh` and CI pick files via `git ls-files`: a **new file is
skipped until it's `git add`-ed**, so the check says OK locally and CI then
fails on it. Stage new files before running the checks.

## CI (since 2026-09-25)
`.github/workflows/ci.yml`: on every push/PR, in the `espressif/idf:latest`
container (= ESP-IDF master, what this is developed on): firmware build,
`tools/size_check.sh`, `tools/format.sh --check`, `tools/test.sh`,
`tools/lint.sh`. The gitignored
`*_secrets.h` are replaced by their `.example` templates there. Locally, run
the same scripts before pushing. The image's ESP-IDF is newer than the
local checkout, so CI's firmware comes out ~15 KB bigger (996 vs 981 KB on
2026-09-25): judge the flash budget by CI's number.

## Static analysis (since 2026-09-25)
`tools/lint.sh` runs clang-tidy (checks in `.clang-tidy`, the detekt config
here; every exclusion there says why) on all tracked `main/*.c`; any finding
fails it. clang-tidy needs clang-compatible flags, so the script configures
a separate `build/clang` tree with `IDF_TOOLCHAIN=clang` and **its own copy
of `sdkconfig`**: configuring clang against the shared `./sdkconfig` flips
its toolchain options, and the next normal build then recompiles everything.
The firmware itself is still built by gcc in `build/`. Tried and dropped:
feeding the gcc `build/compile_commands.json` to clang-tidy (rewriting the
compiler, stripping gcc-only flags) fails on the C library headers
(picolibc is selected via gcc `-specs=`), so the clang tree is required. A finding that's a
false positive gets `// NOLINTNEXTLINE(<check>): <reason>` (see
`weather_parse.c`: the analyzer can't see into cJSON.c). Two that bit new
code (2026-09-25): a `va_start`/`vsnprintf`/`va_end` helper got a
`valist.Uninitialized` false positive (`metrics_format.c` uses a `PUT()`
macro around `snprintf` instead), and assigning an `int8_t` (e.g. WiFi
`rssi`) to `int` trips `bugprone-signed-char-misuse` unless cast
explicitly. `main/CMakeLists.txt` `REQUIRES` is explicit: a new ESP-IDF
header (e.g. `esp_timer.h`) fails the gcc build until its component is
added there.
**Never configure clang against the shared `./sdkconfig`** (e.g. a bare
`idf.py -D IDF_TOOLCHAIN=clang reconfigure`): besides the toolchain it
switches the C library from picolibc to newlib (`CONFIG_LIBC_NEWLIB=y`), and
that sticks after going back to gcc. Newlib is bigger: the firmware grew
~55 KB and overflowed the 1 MB app partition (2026-09-25). Fix: regenerate
`sdkconfig` (delete it, `idf.py build`) or set `CONFIG_LIBC_PICOLIBC=y`.

## Host unit tests (since 2026-09-25)
`tools/test.sh` (after sourcing `export.sh`) builds `test/test_<name>.c`
against `main/<name>.c` with the PC's gcc, ASan/UBSan on, and runs it:
the `./gradlew test` here. Unity (the C test framework) comes from
`$IDF_PATH`, cJSON from `managed_components/` (exists after one `idf.py
build`). Only pure logic is testable this way, so code worth testing goes in
files with **no ESP-IDF includes**: that's why JSON parsing lives in
`weather_parse.c` / `bvg_parse.c`, split from the HTTP fetch in
`weather.c` / `bvg.c` (same for `clock_time.c` / `clock.c`,
`encoder_decode.c` / `encoder.c`, `font.c` / `display.c`), and what each screen shows lives in `screens.c`
(`screen_layout()` fills text rows from a `screen_data_t`; `main.c` only
draws them). New screen = a `screen_t` value + a `layout_*()` + a test. A test that
needs a second pure source (e.g. `test_screens` → `air_parse.c`) gets it
from `extra_sources()` in `tools/test.sh`. Fixtures in `test/fixtures/` (see its README:
`bvg_ok.json` is hand-written, the wrapper was down).

## Serial monitoring
`idf.py monitor` doesn't work in Claude Code (no TTY). Use
**`tools/serial_log.py [seconds] [regex]`** with the IDF python env
(`~/.espressif/python_env/idf6.2_py3.14_env/bin/python`); it resets the board.
How to use it and its traps (empty first read, panic tags in the regex, checks
that need the user's hands, ask what the screen shows when a capture looks
wrong): `../grappim-watcher/docs/esp32/FIRMWARE_PLAYBOOK.md`.

## Build milestones (rough order)
1. ✅ I2C bring-up — SSD1306 shows static text (2026-09-19)
2. ✅ WiFi station connects, logs IP in monitor (2026-09-19)
3. ✅ NTP sync, live clock rendered on screen (2026-09-19)
4. ✅ HTTP + JSON weather fetch, rendered alongside the clock (2026-09-19)
5. 🔶 Polish — in progress: weather icon (sun/cloud/rain/snow/storm, 8x8, mapped
   from Open-Meteo's WMO weathercode) and 15-min refresh cadence done
   (2026-09-19); layout is single always-on screen, not cycling. Icon+temperature
   now drawn side by side on one row via `display_draw_icon_and_text()` rather than
   stacked on separate pages (2026-09-20). Network robustness added (2026-09-20):
   NTP sync retries with backoff (**the retry was a no-op until 2026-09-23**:
   `esp_netif_sntp_init()` refuses a second init, so every retry after a failure
   errored "already initialized"; now deinit-ed on failure) instead of `ESP_ERROR_CHECK`-aborting the device
   on a transient failure right after WiFi comes up (this used to crash-loop the
   board on a slow/flaky network); weather fetch backs off from 30s toward the
   normal 15-min cadence on failure instead of leaving a stale reading up for the
   full interval. DS3231 wired in as the boot time source (2026-09-23). Boot
   **status screen** (2026-09-23): HELLO + a two-column grid of `NAME OK` cells
   (`SPLASH_*` in `main/main.c`: OLED|PWR, RTC|BME on pages 2-3; WIFI|NTP,
   WEATHER on pages 5-6) going `--` → `OK`/`NO`, then the main screens straight after the
   last step (the 3s hold was dropped 2026-09-25). Since 2026-09-24 there are
   three, switched with the KY-040 (turn = next/prev, wraps; press = panel
   off/on since 2026-09-25, was HOME; a turn while off only wakes it):
   page 0 of every screen is time flush left + date `DD/MM/YYYY` flush right;
   HOME has the DWD warning if any, else the next holiday (p1, ROADMAP tasks 9/10), icon+outdoor temp
   (p3), `IN 25C 43H` (p5) and the rain hint (p7, see ROADMAP task 6), OUTDOOR has
   sunrise/sunset and wind/today's max UV, AIR (since 2026-09-25) has the
   European AQI and the two strongest pollen types, INDOOR has temp/humidity and whole
   hPa pressure (font has no `%`/`.`). `draw_screen()` rewrites pages 1-7 in
   full on every change (blank pages via `display_draw_text(page, "")`), so
   switching needs no `display_clear()` and doesn't flicker. The main loop
   waits on the encoder event queue until the next 1s tick, so turns react
   immediately; a quick spin's queued clicks are applied, then drawn once. Boot also runs power/bus
   guardrails (2026-09-23): `PWR OK/NO` status cell (brownout reset reason) and an I2C scan
   against `KNOWN_I2C` — see Power & bus budget. `WEATHER NO` only reflects the first fetch — a transient failure
   there is normal and the 30s retry fills it in. Still open: anything further
   under Open questions below.

## Enclosure (ROADMAP 15, since 2026-09-28)
OpenSCAD (local 2021.01) in `enclosure/`, see its README. `export.sh
[file.scad] [var=value]` = clash check + STLs + PNG renders; `cardboard.py` =
1:1 A4 mock-up templates from the model. Modelling, clash-check and
printability lessons: `../grappim-watcher/docs/esp32/ENCLOSURE_PLAYBOOK.md`;
part dimensions and how each part is held: the part sheets.
- v1 put the ESP32 across the 4 cm side of the 4 x 6 cm perfboard (14 holes,
  a pin row is 15); the user's cardboard mock-up caught it. v2 places ESP32,
  board outline and sockets on one hole grid.

## Roadmap
Next tasks live in `ROADMAP.md` as a checklist, one task per session — read it
at the start of a session and tick items off there when done.

## Open questions
- **Battery/accumulator autonomy** (raised 2026-09-17): user wants the option to run
  untethered, at least for short gaps — not fully scoped yet. Leaning toward a small
  LiPo + TP4056 charge module sized as backup/short-gap runtime (keeps the "always
  on" display concept intact) rather than a full multi-day-portable redesign, but
  not decided or ordered. Revisit once the base build works.
  Worked out 2026-09-28 (still not ordered, user will come back to it):
  a bare LiPo + TP4056 won't do: the DevKit's AMS1117 needs ~4.3 V+ on
  VIN and a LiPo gives 4.2-3.0 V, and a TP4056 can't power the load while
  charging. Plan instead: a LiPo UPS/power-bank module with 5 V output and
  pass-through charging (IP5306-type or MH-CD42) -> ESP32 VIN; the case's
  USB-C hole moves to that module (ESP32's own USB stays inside, for
  flashing with the case open). Cell: flat 803040 (~1000 mAh, ~6 h) or
  103450 (~2000 mAh, ~11 h; case maybe 5-10 mm deeper). Place: back-left
  upper corner above the DS3231, beside the knob (~35 x 35 x 20 mm free),
  away from the sensors and not over the ESP32's top vents. Optional
  divider to D34/D35 (ADC1) for a battery level on screen.
  **Parts chosen 2026-09-28; cell bought, module + USB-C ordered
  2026-10-03** (their part sheets/inventory entries still to come):
  - Cell (bought 2026-10-03, not wired yet): LiPo 103745, 3.7 V
    2000 mAh (~8-9 h), protection board, JST-PH 2.0 plug (red +, black -),
    **47 x 37 x 10 mm** (seller, not measured). Amazon.de B0CSSK9XJR.
    Sheet: `../grappim-watcher/docs/esp32/parts/lipo-103745.md`. Replaces
    the planned Soldered 803160 (1800 mAh, 60 x 31.4 x 8). Too long for
    the 35 x 35 corner above: needs a new spot (flat on the floor /
    against the back wall); case maybe 5-10 mm deeper.
  - Module (ordered): generic "5V 2A integrated charging discharge module"
    (IP5306-type): pads VIN/GND (charge in), BAT/GND, VOUT/GND (5 V ->
    ESP32 VIN), KEY (low pulse: once = on, twice = off). Auto-off under
    50 mA load (we draw ~150 mA, fine). **Plugging the charger in cuts
    VOUT for ~0.3 s -> the ESP32 reboots once** (accepted; RTC keeps time).
  - USB-C breakout (ordered; PENGLIN, red, 6-pin VBUS/GND/CC1/CC2/D+/D-) with
    5.1k CC resistors onboard ("512"), so C-to-C chargers work. VBUS ->
    module VIN, GND -> GND, rest unconnected. This is the case's USB-C hole.
  - JST-PH 2.0 2-pin pigtail -> module BAT/GND, only if the module side
    wants a plug (the cell has one); check red/black polarity against the
    cell before plugging in.
  - On hand: slide switch (VOUT -> ESP32 VIN, check it's rated >= 0.5 A),
    2x 100k for the optional D34 divider.
  - Berrybase equivalents (one-shop order; user wary of LiPos from
    Amazon): module = CHB-214 "4in1 ... 5V / 2A" (same IP5306-type),
    USB-C = Adafruit ADA4090 ("Downstream", has the CC resistors). Upgrade
    that avoids the plug-in reboot: Adafruit PowerBoost 1000 Charger
    (ADA2465, load sharing, micro-USB). Optional I2C fuel gauge instead
    of the divider: Soldered BQ27441 (SOL-333065).
  - If the plug-in reboot ever bothers: diode-OR (USB-C 5 V and module
    VOUT each through a 1N5819 into ESP32 VIN, ~4.7 V at VIN), or a
    1 F / 5.5 V supercap on VIN (needs inrush limiting). Unplug side not
    checked for a gap.
- **WiFi connect timeout** (done 2026-09-25): `wifi_connect()` waits at most
  `WIFI_CONNECT_TIMEOUT_SECONDS` (15s), then boot shows `WIFI NO`/`NTP NO`/
  `WEATHER NO` and goes to the main screen on RTC time. WiFi keeps retrying
  in the background; the main loop runs NTP (every `NTP_RETRY_SECONDS`) and
  the weather fetch once `wifi_is_connected()`. Offline boot verified with a
  fake SSID; the catch-up once WiFi appears *after* boot was not tested
  on hardware (dropped, not needed).

## Relationship to `esp32` lessons repo
Separate git repo at `~/proj/esp32` (not a sibling of this folder), not a
subfolder of it — this is meant to be a standalone real build, not another
numbered lesson. Reuses concepts learned there (GPIO setup, debounce patterns,
`vTaskDelay` discipline) but doesn't share code.

## Relationship to `esp32-hw-checks`
Sibling folder (`../esp32-hw-checks`), created 2026-09-19: standalone
bring-up/test firmware for verifying boards and modules in isolation before
they're trusted in this firmware. A new sensor gets a check there first; what
each check does and when a part passed is in its part sheet. It is a git repo
since 2026-10-02 (local only, no remote) with its own `CLAUDE.md`: commit
changes there too.
