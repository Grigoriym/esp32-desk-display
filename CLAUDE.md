# ESP32 Desk Info Display

## Concept
A WiFi-connected desk companion on a small OLED screen: live clock (NTP) + current
weather, always on. First "real" project after the esp32 lessons repo — not a
tutorial exercise, meant to actually sit on the desk.

## Hardware
- **Board**: same ESP32 DevKit used in the `esp32` lessons project (GPIO2 onboard
  LED, flashes on `/dev/ttyUSB0`) for this build. No new board needed for v1.
- **Display**: 0.96" 128x64 I2C OLED module, **SSD1315 controller** (confirmed from
  listing, SSD1306-command-compatible — SSD1306 drivers/init sequence work
  unmodified). Ordered 2026-09-17, see Shopping list below.
- **Wiring plan**: I2C on GPIO21 (SDA) / GPIO22 (SCL) — ESP32's conventional default
  I2C pins, unused by any lesson so far. VCC/GND from the board's 3V3/GND rail.
- Breadboard, jumper wires, USB cable — already on hand from the Freenove kit.
- **Hardware confirmed working (2026-09-19)**: all 3 ESP32 boards and all 3 OLED
  panels from the shopping list tested individually (LED blink, I2C scan, OLED
  fill/text) via the `esp32-hw-checks` sibling project (see below) — no DOA units,
  no substitutions needed. OLED always responds at I2C address 0x3C.
- The final soldered board+OLED needed the panel mounted physically upside-down;
  fixed in software, not wiring — see Display driver notes below.
- **As-built wiring (2026-09-23)**: OLED and DS3231 are each wired **directly** to
  the ESP32 in parallel (both on 3V3/GND/D21/D22), not daisy-chained. While chained
  through the DS3231's 4-pin pass-through, one module setup left the OLED answering
  but no 0x68, another took the whole bus down (nothing answered); root cause not
  pinned down (wiring/joints, not the modules), the parallel wiring just worked.
- **Pin table for every module lives in `docs/WIRING.md`**; keep it updated
  when wiring changes. Build principle: every module sits in its own female
  socket on the perfboard so it can be pulled and swapped. The ESP32 itself
  plugs *down* into sockets, so its pins aren't reachable from above for a
  jumper: a new module means soldering a new socket. The board may get
  redesigned (2026-09-23).
- LEDs: the red LED on the ESP32 DevKit and the red LED on the DS3231 are both
  plain **power** indicators (not error/short signs). The blue LED is GPIO2 —
  blinked by `esp32-hw-checks`, off in this firmware.
- **The OLED keeps its last image while powered, even across an ESP32 reset** —
  a leftover "HELLO" on screen does *not* prove the OLED is on the bus. To test
  the bus, trust the I2C scan (or power-cycle the board, not just reset it).

## Shopping list — ordered 2026-09-17 (sourced away from AliExpress, brand-name
sellers, to avoid EU import duty on AliExpress orders; exact per-item source not logged, just
that it wasn't raw AliExpress)
- [x] 0.96" 128x64 I2C OLED breakout, 4-pin (VCC/GND/SCL/SDA), **SSD1315
  controller confirmed** (SSD1306-compatible) — APKLVSR, pack of 3, blue
- [x] BME280 (temp/humidity/pressure, I2C) — 5V-labeled board, but schematic shows
  onboard 3.3V LDO + BSS138 level shifters, so **wire its VIN to the ESP32's 3.3V
  pin** (not 5V) to keep the I2C lines at safe logic level. Board silkscreen says
  "BME/BMP 280" (generic PCB for either chip); **confirmed real BME280
  2026-09-23** via chip ID 0x60 (BMP280 would be 0x58) — check lives in
  `esp32-hw-checks`. Wired in parallel on the bus (VIN→3V3, SDA→21, SCL→22),
  answers at **0x76**; read by this firmware since 2026-09-23 (`main/bme280.c`)
- [x] DS3231 RTC module (I2C, coin-cell backed, AT24C32 EEPROM bonus onboard) —
  APKLVSR, pack of 3; power VCC from 3.3V not 5V for the same I2C-safety reason.
  Module has two headers: a 6-pin one (32K, SQW, SCL, SDA, VCC, GND) and a 4-pin
  pass-through (SCL, SDA, VCC, GND) wired in parallel on the PCB, for daisy-chaining
  a second I2C device without a breadboard tie point. Wiring plan: 6-pin header to
  the ESP32 (SDA→GPIO21, SCL→GPIO22, VCC→3.3V, GND→GND — same bus as the OLED,
  different address: OLED 0x3C, DS3231 0x68), then the 4-pin pass-through straight
  to the OLED's four pins. 32K and SQW left unconnected (not needed for basic
  timekeeping). Soldered and working 2026-09-23 (two of the three modules tested,
  both fine — earlier "no 0x68" failures were wiring, not DOA units); in the end
  wired in parallel rather than through the pass-through, see Hardware above. On
  the scan it answers at **0x68** (clock), **0x57** (AT24C32 EEPROM) and **0x5F**
  (extra address some of these modules expose — normal, not a fault).
- [x] Rotary encoder (KY-040) — GIAK, pack of 5; candidate UI input for cycling
  screens/adjusting brightness; one is wired (D25/D26/D27) and drives the
  screen switching since 2026-09-24
- [x] ESP32-WROOM-family DevKit boards, onboard PCB antenna, USB-C, CP2102 — ELEGOO
  "ESP-32S", pack of 3, 30-pin, 4MB flash; bulk restock, same chip family as the
  current lesson board (exact WROOM-32D marking unconfirmed but functionally
  equivalent for this project either way)
- [x] Perforated/prototyping PCB grid board kit (Miuzei, 78-piece, 21 double-sided
  boards) — not in the original plan, general prototyping stock for soldering up
  the OLED/BME280/RTC wiring once breadboarding is done
- [ ] 1-2x ESP32-WROVER-B DevKit (onboard antenna, PSRAM) — note the Freenove kit
  (see `esp32` lessons repo) already includes an **ESP32-WROVER** board with PSRAM,
  so one is on hand before ordering — held in reserve for a
  future color-TFT/LVGL project, not this one — not ordered yet
- MQ-135 (air quality) and RC522 (RFID) were discussed as optional future adds —
  not part of this order
- 2.4"-3.5" ILI9341/ILI9488 TFT (touch) — discussed as a **future** upgrade path
  for a color-graphics version of this project, not part of this order

**Parts arrived and confirmed (2026-09-19)**: BOM matches what's checked off above,
no substitutions, all 3 ESP32 boards and 3 OLEDs tested working (see Hardware
section above). Milestones 1-4 built on this hardware.

## Power & bus budget (guardrail — check before wiring any new module)
Everything runs from the DevKit's 3V3 pin (onboard regulator, likely AMS1117 —
~500 mA safe continuous from USB, which itself caps at ~500 mA).

| Part | Typical | Peak | Notes |
|---|---|---|---|
| ESP32 + WiFi | 100-150 mA | ~350 mA | TX bursts; the dominant load |
| OLED SSD1315 | 10-15 mA | ~25 mA | scales with lit pixels |
| DS3231 module | 1-2 mA | ~3 mA | mostly the power LED |
| BME280 | 1-2 mA | ~3 mA | LDO + LED; sensor itself is µA |
| KY-040 | ~0.3 mA | ~1 mA | pull-ups only |
| **Total** | **~130-170 mA** | **~380 mA** | ~100+ mA headroom left |

Rules:
- New module → add a row above, re-check the total stays under ~450 mA peak.
- **I2C pull-ups add up in parallel**: each module brings its own (4.7k/10k).
  Keep combined ≥ ~1.1k (3 mA sink limit) — roughly 5-6 modules max before
  removing pull-ups from some boards. Keep bus wires short.
- New I2C address → check it doesn't clash, then add it to `KNOWN_I2C` in
  `main/main.c` (boot log warns on any unknown address).
- All I2C modules powered from 3.3V, never 5V (logic level + the DS3231
  module's coin-cell "charging" circuit, which would overcharge a plain CR2032
  on 5V). GPIOs are signals only (~12 mA each), never power.
- 5V-hungry parts go on VIN, not 3V3: MQ-135 heater ~150 mA @5V (and its
  analog out needs a divider); TFT backlight 50-100 mA.
- Battery (open question below): ~150 mA average → 1000 mAh ≈ 6-7 h.
- Runtime check: status screen `PWR NO` / log `last reset was a BROWNOUT`
  means the supply sagged (overload or weak USB port/cable) — ESP-IDF's
  brownout detector is on by default (`CONFIG_ESP_BROWNOUT_DET`).

## Software / ESP-IDF components used
| Need | Component |
|---|---|
| I2C bus | `esp_driver_i2c` |
| BME280 | hand-rolled driver (`main/bme280.c`): chip-ID check, factory calibration, Bosch datasheet integer compensation, forced mode x1 oversampling |
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
| Phone page + JSON API | `esp_http_server` + mDNS (`espressif/mdns`, component manager) in `main/web.c`: `http://desk.local`, page embedded from `main/web_index.html` (`EMBED_TXTFILES`), JSON built by the pure `main/web_api.c` (host-tested). Commands go to the main loop through the encoder queue (`input_event_t`, `main/input.h`). Endpoints in README "Phone access and API". No auth, LAN only. Since 2026-09-26 (ROADMAP 11) |
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
- **`CONFIG_FREERTOS_HZ=100`** here and in `esp32-hw-checks`: one tick is
  10 ms, so `pdMS_TO_TICKS(<10)` is 0 and `vTaskDelay(0)` busy-loops. That's
  why the KY-040 check decodes the quadrature in a GPIO any-edge ISR (10 ms
  polling would drop steps on a quick spin) and only polls the button.
- **Pure logic is checked on the host before flashing** by `tools/test.sh`
  (see Host unit tests), incl. `utc_to_epoch()` and the TZ rule's switch
  dates (`test/test_clock_time.c`, against glibc: it checks the rule string,
  not picolibc's parser of it).
- **Brightness can't be dimmed on this SSD1315** (tested 2026-09-25):
  contrast (`0x81`) 0x01-0x40 look the same, 0x00 turns the panel off,
  0xFF is only slightly brighter; lowering pre-charge (`0xD9`) / VCOMH
  (`0xDB`) together with contrast 0 also gives black. Effectively on/off
  only. Contrast is 0x40 (was 0xCF, no visible difference).
- Panel orientation: `0xA0`/`0xC0` (segment remap / COM scan) in the init sequence
  is flipped 180° from the SSD1306 default, to match how the OLED ended up mounted
  once soldered to the perfboard (upside-down relative to native wiring). If a new
  panel is soldered in a different orientation, flip these two bytes, not the wiring.

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
`. ~/esp/esp-idf/export.sh && idf.py -p /dev/ttyUSB0 build flash` (same for
`../esp32-hw-checks`). Swapping between the two projects' firmware on one board is
routine during hardware debugging — remember to flash this project back after.
After such a swap, esptool may print "Verification failed after fast reflash ...
Reflashing the whole image" — harmless, it recovers on its own and ends `Done`.
`A fatal error occurred: No serial data received` from the flash step
was transient (2026-09-25): the same command worked on the next try.
**Check the flash actually happened** before trusting a device test: when
filtering `idf.py` output, grep case-insensitively (`-iE "error|failed|Done"`).
The partition-overflow failure prints `Error: app partition is too small`
and no `Done`; a case-sensitive `error` filter hid it on 2026-09-25, the old
firmware kept running, and a device check "passed" against it. The boot
log's `app_init: App version: <short hash>[-dirty]` / `Compile time` lines
say which build is actually running.

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

## Serial monitoring gotcha (this dev harness)
`idf.py monitor` fails here with "Monitor requires standard input to be attached to
TTY" — the Claude Code Bash tool isn't a real terminal. Workaround used this session:
read the port directly with pyserial via the IDF python env, e.g.
`~/.espressif/python_env/idf6.2_py3.14_env/bin/python` opening `/dev/ttyUSB0` at
115200 and looping `read()`. **This capture method is unreliable** — it repeatedly
produced huge garbled/duplicated bursts (once 13MB from a 15-second read, physically
impossible over 115200 baud) that looked like device crash-loops but weren't; the
device was fine every time, confirmed by just asking what the physical screen showed.
Prefer asking the user for ground truth (what's on screen) over trusting these
captures when something looks wrong. 2026-09-23 update: a short capture (toggle RTS
to reset, read 5-20s, cap the buffer, grep for the log tags you care about) was
reliable all session. The first read right after the board re-enumerates on USB
(replug/rewire) **or right after a flash** sometimes returns nothing at all — just
retry once. This is packaged as **`tools/serial_log.py [seconds] [regex]`** (run
with the IDF python env above): resets via RTS, caps the buffer, greps, retries
once if empty. Use it instead of ad-hoc pyserial snippets. For tests that need
the user to act (turn the knob, press a button), list the steps in the reply
*before* starting a ~40s capture, then grep for the relevant tags; this worked
first time for both encoder tests on 2026-09-24.

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

## Roadmap
Next tasks live in `ROADMAP.md` as a checklist, one task per session — read it
at the start of a session and tick items off there when done.

## Open questions
- **Battery/accumulator autonomy** (raised 2026-09-17): user wants the option to run
  untethered, at least for short gaps — not fully scoped yet. Leaning toward a small
  LiPo + TP4056 charge module sized as backup/short-gap runtime (keeps the "always
  on" display concept intact) rather than a full multi-day-portable redesign, but
  not decided or ordered. Revisit once the base build works.
- **WiFi connect timeout** (done 2026-09-25): `wifi_connect()` waits at most
  `WIFI_CONNECT_TIMEOUT_SECONDS` (15s), then boot shows `WIFI NO`/`NTP NO`/
  `WEATHER NO` and goes to the main screen on RTC time. WiFi keeps retrying
  in the background; the main loop runs NTP (every `NTP_RETRY_SECONDS`) and
  the weather fetch once `wifi_is_connected()`. Offline boot verified with a
  fake SSID; the catch-up once WiFi appears *after* boot was not tested
  on hardware (dropped, not needed).

## Relationship to `esp32` lessons repo
Separate git repo, not a subfolder of the lessons project — this is meant to be a
standalone real build, not another numbered lesson. Reuses concepts learned there
(GPIO setup, debounce patterns, `vTaskDelay` discipline) but doesn't share code.

## Relationship to `esp32-hw-checks`
Sibling folder (`../esp32-hw-checks`, not a subfolder), created 2026-09-19. Holds
standalone bring-up/test firmware for verifying ESP32 boards and modules/sensors in
isolation (LED blink + I2C scan + OLED fill/text test + BME280 chip-ID check
(0x60 BME280 vs 0x58 BMP280, added 2026-09-23) + KY-040 encoder on D25/D26/D27
(passed 2026-09-24) + LDR raw ADC readout on D34 (added 2026-09-25; the LDR is no longer on the
desk-display board); add a check there for each new sensor as it gets wired up) before that hardware is trusted
enough to use in this project's real firmware. **It is not a git repo** — its
changes exist only on disk, so there's nothing to commit there.
