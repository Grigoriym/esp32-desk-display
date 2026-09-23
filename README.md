# ESP32 Desk Display

A small always-on desk companion: an ESP32 drives a 0.96" OLED that shows the
current time and outdoor weather. It syncs the clock over NTP, keeps time
through power loss with a battery-backed DS3231 RTC, and fetches weather from
[Open-Meteo](https://open-meteo.com/) (free, no API key).

Written in C on ESP-IDF, with a small hand-rolled SSD1306 driver and no
graphics library.

## Features

- **Clock**: `HH:MM` plus the date as `DD/MM/YYYY`. The RTC sets it at boot, NTP corrects it, and the RTC is
  updated after every sync.
- **Weather**: current temperature plus an 8x8 icon (sun, cloud, rain, snow or
  storm), refreshed every 15 minutes. A failed fetch is retried after 30s, then
  at growing intervals up to the normal 15 minutes.
- **Boot status screen**: shows each subsystem coming up, with `--` changing to
  `OK` or `NO`:

  ```
         HELLO

      OLED    OK
      RTC     OK
      WIFI    OK
      NTP     OK
      WEATHER OK
  ```

- **Degrades gracefully**: without an RTC, the clock waits for NTP. Without
  NTP, the RTC time is used.

## Hardware

| Part | Notes |
|---|---|
| ESP32 DevKit (WROOM-32, 30-pin) | Any ESP32 with GPIO21/22 free works |
| 0.96" 128x64 I2C OLED, SSD1306/SSD1315 | Address `0x3C` |
| DS3231 RTC module with coin cell | Address `0x68`. Optional but recommended |

All modules share one I2C bus. Wire each one **directly to the ESP32, in
parallel**:

| Module pin | ESP32 pin |
|---|---|
| VCC | 3V3 (not 5V) |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

## Build and flash

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) (developed
on v6.1-dev).

```sh
cp main/wifi_secrets.h.example main/wifi_secrets.h   # then fill in your SSID/password
. ~/esp/esp-idf/export.sh
idf.py -p /dev/ttyUSB0 build flash monitor
```

`main/wifi_secrets.h` is gitignored, so your credentials never get committed.
cJSON is pulled in automatically by the component manager.

## Configuration

Settings are hardcoded constants. Change them and reflash:

| What | Where |
|---|---|
| Weather location (default: Berlin) | `WEATHER_URL` in `main/weather.c` |
| UTC offset, no automatic DST | `UTC_OFFSET_HOURS` in `main/clock.c` |
| Weather refresh interval | `WEATHER_REFRESH_SECONDS` in `main/main.c` |
| Boot screen hold time | `SPLASH_HOLD_SECONDS` in `main/main.c` |
| Display orientation | `0xA0`/`0xC0` in the init sequence in `main/display.c` (this build's panel is mounted upside-down; use `0xA1`/`0xC8` for the default orientation) |

## Project layout

```
main/
  main.c      boot sequence, status screen, main loop
  display.c   SSD1306 driver, 5x7 font, text/icon drawing
  clock.c     NTP sync, DS3231 read/write, time formatting
  weather.c   Open-Meteo HTTPS fetch, JSON parsing, weather icons
  wifi.c      WiFi station connect
```

Hardware bring-up tests (I2C scan, OLED test pattern) live in a separate
sibling project, `esp32-hw-checks`.

## Known limitations

- No automatic daylight-saving time, so `UTC_OFFSET_HOURS` is flipped by hand
  twice a year.
- WiFi connect has no timeout. With no network, the boot screen waits at
  `WIFI --`.

## License

[Apache License 2.0](LICENSE)
