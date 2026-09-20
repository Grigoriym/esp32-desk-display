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

## Shopping list — ordered 2026-09-17 (sourced away from AliExpress, brand-name
sellers, due to the EU duty tax note below; exact per-item source not logged, just
that it wasn't raw AliExpress)
- [x] 0.96" 128x64 I2C OLED breakout, 4-pin (VCC/GND/SCL/SDA), **SSD1315
  controller confirmed** (SSD1306-compatible) — APKLVSR, pack of 3, blue
- [x] BME280 (temp/humidity/pressure, I2C) — 5V-labeled board, but schematic shows
  onboard 3.3V LDO + BSS138 level shifters, so **wire its VIN to the ESP32's 3.3V
  pin** (not 5V) to keep the I2C lines at safe logic level; verify it's a real
  BME280 not a mislabeled BMP280 once wired up (humidity should actually read)
- [x] DS3231 RTC module (I2C, coin-cell backed, AT24C32 EEPROM bonus onboard) —
  APKLVSR, pack of 3; power VCC from 3.3V not 5V for the same I2C-safety reason
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

## Software / ESP-IDF components used
| Need | Component |
|---|---|
| I2C bus | `esp_driver_i2c` |
| OLED framebuffer + text rendering | hand-rolled minimal SSD1306 driver (`main/display.c`) — decided against a component-manager package |
| WiFi station | `esp_wifi`, `esp_netif`, `nvs_flash` |
| Clock sync | SNTP (`esp_netif_sntp`) |
| Weather fetch | `esp_http_client` + `mbedtls` (`esp_crt_bundle_attach` for TLS, Open-Meteo is HTTPS-only) |
| JSON parsing | `cJSON` — **not bundled** in this ESP-IDF version (v6.1-dev); pulled via the component manager (`main/idf_component.yml` → `espressif/cjson`), lands in gitignored `managed_components/` |

## Display driver notes (`main/display.c`)
- Font and weather icons are **hand-derived bitmaps**, one glyph at a time, with no
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
captures when something looks wrong.

## Build milestones (rough order)
1. ✅ I2C bring-up — SSD1306 shows static text (2026-09-19)
2. ✅ WiFi station connects, logs IP in monitor (2026-09-19)
3. ✅ NTP sync, live clock rendered on screen (2026-09-19)
4. ✅ HTTP + JSON weather fetch, rendered alongside the clock (2026-09-19)
5. 🔶 Polish — in progress: weather icon (sun/cloud/rain/snow/storm, 8x8, mapped
   from Open-Meteo's WMO weathercode) and 15-min refresh cadence done
   (2026-09-19); layout is single always-on screen (icon + clock + temperature
   stacked), not cycling. Still open: anything further under Open questions below.

## Open questions
- **Battery/accumulator autonomy** (raised 2026-09-17): user wants the option to run
  untethered, at least for short gaps — not fully scoped yet. Leaning toward a small
  LiPo + TP4056 charge module sized as backup/short-gap runtime (keeps the "always
  on" display concept intact) rather than a full multi-day-portable redesign, but
  not decided or ordered. Revisit once the base build works.
- Rotary encoder, DS3231, and BME280 aren't wired into the app yet (milestones 1-4
  only used the ESP32 + OLED) — fold in as a later milestone. Screen layout is
  currently a single static screen; revisit if/when local-sensor data needs to
  join the rotation.
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
