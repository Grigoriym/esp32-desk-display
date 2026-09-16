# ESP32 Desk Info Display

## Concept
A WiFi-connected desk companion on a small OLED screen: live clock (NTP) + current
weather, always on. First "real" project after the esp32 lessons repo — not a
tutorial exercise, meant to actually sit on the desk.

## Hardware
- **Board**: same ESP32 DevKit used in the `esp32` lessons project (GPIO2 onboard
  LED, flashes on `/dev/ttyUSB0`) for this build. No new board needed for v1.
- **Display**: SSD1306 or SH1106 128x64 I2C OLED module — **ordered 2026-09-16**,
  controller chip (SSD1306 vs SH1106) unconfirmed until it arrives — check before
  picking/writing the driver, see Open Questions.
- **Wiring plan**: I2C on GPIO21 (SDA) / GPIO22 (SCL) — ESP32's conventional default
  I2C pins, unused by any lesson so far. VCC/GND from the board's 3V3/GND rail.
- Breadboard, jumper wires, USB cable — already on hand from the Freenove kit.

## Shopping list — ordered 2026-09-16 (AliExpress, exact items/qty TBD until arrival)
- [x] SSD1306/SH1106 128x64 I2C OLED breakout, 4-pin (VCC/GND/SCL/SDA), 3.3V version
- [x] BME280 (temp/humidity/pressure, I2C, **3.3V version** — not the 5V board) —
  watch for the common AliExpress mislabeling scam where BMP280 (no humidity) ships
  under a "BME280" listing; verify humidity actually reads once wired up
- [x] DS3231 RTC module (I2C, coin-cell backed) — keeps time without WiFi/NTP
- [x] Rotary encoder (KY-040) — candidate UI input for cycling screens/adjusting
  brightness, not yet wired into any milestone
- [ ] ESP32-WROOM-32D DevKit boards, onboard PCB antenna (no "U" suffix) — bulk
  restock, same chip family as the current lesson board
- [ ] 1-2x ESP32-WROVER-B DevKit (onboard antenna, PSRAM) — held in reserve for a
  future color-TFT/LVGL project, not this one
- MQ-135 (air quality) and RC522 (RFID) were discussed as optional future adds;
  unclear if included in this specific order — confirm when boxes arrive
- 2.4"-3.5" ILI9341/ILI9488 TFT (touch) — discussed as a **future** upgrade path
  for a color-graphics version of this project, not ordered as part of this batch
  as far as this doc knows — confirm

**When parts arrive**: update the checkboxes above, confirm exact BOM, then this
section can shrink back down to just what's actually being wired in.

## Software / ESP-IDF components needed
| Need | Component |
|---|---|
| I2C bus | `esp_driver_i2c` |
| OLED framebuffer + text rendering | undecided — hand-roll a minimal SSD1306 driver, or pull an existing component-manager package (research before committing) |
| WiFi station | `esp_wifi`, `esp_netif`, `nvs_flash` |
| Clock sync | SNTP (`esp_netif_sntp` / lwIP SNTP) |
| Weather fetch | `esp_http_client` (+ `esp-tls` if the API is HTTPS) |
| JSON parsing | `json` (cJSON, bundled with ESP-IDF) |

## Data source decisions (tentative — revisit before building)
- **Weather API**: leaning toward Open-Meteo (free, no signup/API key) over
  OpenWeatherMap (needs account + key) — simpler to get running. Not locked in.
- **Location**: hardcoded lat/long, no GPS.
- **Timezone**: hardcode an offset for now; IP-geolocation is a maybe-later nicety.

## Secrets
WiFi SSID/password must never be committed. Likely a gitignored header
(`wifi_secrets.h`) or Kconfig values via `menuconfig` — not yet decided which.
`.gitignore` already excludes `*_secrets.h` and `sdkconfig` so whichever we pick
is covered.

## Build milestones (rough order)
1. I2C bring-up — SSD1306 shows static "hello world" text
2. WiFi station connects, logs IP in monitor
3. NTP sync, live clock rendered on screen
4. HTTP + JSON weather fetch, rendered alongside the clock
5. Polish — layout, refresh interval, maybe a small icon

## Open questions
- **First thing next session**: confirm which parts actually arrived (SSD1306 vs
  SH1106, whether MQ-135/RC522/TFT were in the order) and update the shopping
  list above from checkboxes to reality.
- Driver: write a minimal SSD1306 driver ourselves (more learning, more control)
  vs. pull an existing ESP-IDF component (faster to a working screen)?
- Screen layout: clock + weather on one screen, or cycling between screens? BME280
  local readings could join the rotation once that sensor's wired up.
- Refresh cadence for the weather fetch (rate limits, battery/heat not a concern
  since this is desk-powered, but no need to hammer the API either).
- Rotary encoder and DS3231 aren't in the milestone list yet — fold in once the
  base clock+weather loop (milestones 1-4) works.

## Relationship to `esp32` lessons repo
Separate git repo, not a subfolder of the lessons project — this is meant to be a
standalone real build, not another numbered lesson. Reuses concepts learned there
(GPIO setup, debounce patterns, `vTaskDelay` discipline) but doesn't share code.
