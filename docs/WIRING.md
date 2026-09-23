# Wiring

All modules share the ESP32's 3V3 and GND. Never 5V/VIN (see "Power & bus
budget" in `CLAUDE.md`). Each module sits in its own female socket on the
perfboard, so it can be pulled out and swapped.

| Module | Module pin | ESP32 pin | Notes |
|---|---|---|---|
| OLED SSD1315 | VCC | 3V3 | |
| | GND | GND | |
| | SDA | D21 | I2C, addr 0x3C |
| | SCL | D22 | |
| DS3231 RTC (6-pin header) | VCC | 3V3 | not 5V: coin-cell charging circuit |
| | GND | GND | |
| | SDA | D21 | I2C, addr 0x68 (+0x57 EEPROM, 0x5F) |
| | SCL | D22 | |
| | 32K, SQW | — | unconnected |
| BME280 | VIN | 3V3 | |
| | GND | GND | |
| | SDA | D21 | I2C, addr 0x76 |
| | SCL | D22 | |
| KY-040 encoder *(planned)* | + | 3V3 | onboard 10k pull-ups go to this pin |
| | GND | GND | |
| | SW | D27 | button, active low |
| | DT | D26 | |
| | CLK | D25 | |

- I2C modules are wired **in parallel** straight to D21/D22, not daisy-chained
  through the DS3231's pass-through header (that setup failed, see `CLAUDE.md`).
- KY-040 plan: a 5-pin female socket on the perfboard in KY-040 pin order
  (GND, +, SW, DT, CLK), then a male-female Dupont cable to the knob so it can
  sit apart from the board. The encoder pins are only `#define`s, so they can
  move to D18/D19/D23 if a board redesign makes the right side more convenient.
- Free, non-strapping GPIOs for future modules: D4, D16, D17, D18, D19, D23,
  D32, D33 (D34/D35/VP/VN are input-only).
