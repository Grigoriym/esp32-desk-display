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
  screens/adjusting brightness, not yet wired into any milestone
- [x] ESP32-WROOM-family DevKit boards, onboard PCB antenna, USB-C, CP2102 — ELEGOO
  "ESP-32S", pack of 3, 30-pin, 4MB flash; bulk restock, same chip family as the
  current lesson board (exact WROOM-32D marking unconfirmed but functionally
  equivalent for this project either way)
- [x] Perforated/prototyping PCB grid board kit (Miuzei, 78-piece, 21 double-sided
  boards) — not in the original plan, general prototyping stock for soldering up
  the OLED/BME280/RTC wiring once breadboarding is done
- [ ] 1-2x ESP32-WROVER-B DevKit (onboard antenna, PSRAM) — held in reserve for a
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
| KY-040 (planned) | ~0.3 mA | ~1 mA | pull-ups only |
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
| OLED framebuffer + text rendering | hand-rolled minimal SSD1306 driver (`main/display.c`) — decided against a component-manager package |
| WiFi station | `esp_wifi`, `esp_netif`, `nvs_flash` |
| Clock sync | SNTP (`esp_netif_sntp`) |
| Weather fetch | `esp_http_client` + `mbedtls` (`esp_crt_bundle_attach` for TLS, Open-Meteo is HTTPS-only) |
| JSON parsing | `cJSON` — **not bundled** in this ESP-IDF version (v6.1-dev); pulled via the component manager (`main/idf_component.yml` → `espressif/cjson`), lands in gitignored `managed_components/` |

## Display driver notes (`main/display.c`)
- Letters A-Z (added 2026-09-23 for the boot status screen) are the classic
  Adafruit GFX glcdfont 5x7 bitmaps, not hand-derived — all rendered correctly
  first time. `C` deliberately kept as the original open-sided variant. Anything
  outside A-Z/0-9/`:`/`-`/`/` (incl. space) renders blank. `/` is also
  glcdfont (added 2026-09-23 for the date).
- Digits and weather icons are **hand-derived bitmaps**, one glyph at a time, with no
  rendering preview before flashing — there's no tooling to check the bit math ahead
  of time. Found one real transcription bug this way (digit '9' had a missing
  mid-row bit, only visible once rendered on real hardware). When adding new
  characters/icons, expect to eyeball the result on the physical panel and fix bits
  as needed, not to get it right blind on the first try.
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
- **Timezone**: hardcoded offset in `main/clock.c` (`UTC_OFFSET_HOURS`), no DST
  logic. Currently `2` (Berlin CEST). **Needs manual flip to `1`** when Berlin's DST
  ends (~last Sunday of October 2026), and back to `2` next spring.
- **Weather refresh cadence**: every 15 minutes (`WEATHER_REFRESH_SECONDS` in
  `main/main.c`), decided 2026-09-19 — gentle on the API, fresh enough for a desk
  display.

## Secrets
Decided: plain gitignored header, `main/wifi_secrets.h` (real credentials, never
committed) with `main/wifi_secrets.h.example` as the committed template. `.gitignore`
already excludes `*_secrets.h` and `sdkconfig`.

## Build / flash
`. ~/esp/esp-idf/export.sh && idf.py -p /dev/ttyUSB0 build flash` (same for
`../esp32-hw-checks`). Swapping between the two projects' firmware on one board is
routine during hardware debugging — remember to flash this project back after.

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
(replug/rewire) sometimes returns nothing at all — just retry once.

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
   WEATHER on pages 5-6) going `--` → `OK`/`NO`, held `SPLASH_HOLD_SECONDS` (3s)
   after the last step, then the main screen. Main screen layout (2026-09-23,
   `MAIN_PAGE_*`): page 0 time flush left + date `DD/MM/YYYY` flush right
   (`display_draw_text_columns()`), page 3 icon+outdoor temp, page 5 indoor
   `IN 26C 34H` (BME280, every 10s; font has no `%`/`.`, pressure log-only),
   pages 6-7 free for more BME-derived data (pressure trend, dew point,
   comfort, min/max were discussed, not chosen yet). Boot also runs power/bus
   guardrails (2026-09-23): `PWR OK/NO` status cell (brownout reset reason) and an I2C scan
   against `KNOWN_I2C` — see Power & bus budget. `WEATHER NO` only reflects the first fetch — a transient failure
   there is normal and the 30s retry fills it in. Still open: anything further
   under Open questions below.

## Open questions
- **Battery/accumulator autonomy** (raised 2026-09-17): user wants the option to run
  untethered, at least for short gaps — not fully scoped yet. Leaning toward a small
  LiPo + TP4056 charge module sized as backup/short-gap runtime (keeps the "always
  on" display concept intact) rather than a full multi-day-portable redesign, but
  not decided or ordered. Revisit once the base build works.
- Rotary encoder isn't wired into the app yet — fold in as a later milestone.
  Screen layout is a single static screen; with BME280 data it's near full,
  so more readings need rotation or encoder-driven screens.
- BME280 temperature may read high from the ESP32/regulator's own heat
  (26.5°C seen on first read, not yet cross-checked against a thermometer).
- **WiFi has no timeout** (seen 2026-09-23, deliberately left alone):
  `wifi_connect()` waits forever, so with no network the status screen sits at
  `WIFI --` indefinitely and the clock never appears, even though the RTC already
  has the right time. Fix: bounded wait, show `WIFI NO`, skip NTP/weather, go to
  the main screen on RTC time, keep reconnecting in the background.
- UTC offset needs a manual flip twice a year (see Data source decisions) — a
  standing maintenance task, not automated.

## Relationship to `esp32` lessons repo
Separate git repo, not a subfolder of the lessons project — this is meant to be a
standalone real build, not another numbered lesson. Reuses concepts learned there
(GPIO setup, debounce patterns, `vTaskDelay` discipline) but doesn't share code.

## Relationship to `esp32-hw-checks`
Sibling folder (`../esp32-hw-checks`, not a subfolder), created 2026-09-19. Holds
standalone bring-up/test firmware for verifying ESP32 boards and modules/sensors in
isolation (LED blink + I2C scan + OLED fill/text test today; add a check there for
each new sensor — BME280, DS3231, encoder — as it gets wired up) before that
hardware is trusted enough to use in this project's real firmware.
