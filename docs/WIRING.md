# Wiring

All modules share the ESP32's 3V3 and GND. Never 5V/VIN (see "Power & bus
budget" in `CLAUDE.md` for this build's total). Everything is currently on a solderless
breadboard; the soldered carrier board comes with the enclosure (ROADMAP
task 15).

Wire colours: 🟢 green = D21 (SDA), 🟡 yellow = D22 (SCL).

| Module | Module pin | ESP32 pin | Notes |
|---|---|---|---|
| OLED SSD1315 | VCC | 3V3 | |
| | GND | GND | |
| | SDA | 🟢 D21 | I2C, addr 0x3C |
| | SCL | 🟡 D22 | |
| DS3231 RTC (6-pin header) | VCC | 3V3 | not 5V: coin-cell charging circuit |
| | GND | GND | |
| | SDA | 🟢 D21 | I2C, addr 0x68 (+0x57 EEPROM, 0x5F) |
| | SCL | 🟡 D22 | |
| | 32K, SQW | — | unconnected |
| BME280 | VIN | 3V3 | |
| | GND | GND | |
| | SDA | 🟢 D21 | I2C, addr 0x76 |
| | SCL | 🟡 D22 | |
| SCD41 CO2 | GND | GND | pin order on the module: GND, VDD, SCL, SDA |
| | VDD | 3V3 | |
| | SCL | 🟡 D22 | I2C, addr 0x62 |
| | SDA | 🟢 D21 | |
| KY-040 encoder | + | 3V3 | onboard 10k pull-ups go to this pin |
| | GND | GND | |
| | SW | D27 | button, active low |
| | DT | D26 | |
| | CLK | D25 | |

- Part facts (module pin order, addresses, current draw, the DevKit's pin order)
  are in each part's Homebox entry (`python3
  ../grappim-watcher/docs/esp32/inventory/parts.py show <part>`); GPIO, power and I2C rules in
  `../grappim-watcher/docs/esp32/WIRING_RULES.md`. This file is only this
  build's pin table.
- I2C modules are wired **in parallel** straight to D21/D22 (rule and the
  failed daisy-chain: `WIRING_RULES.md` there, and the DS3231's Homebox entry).
- KY-040: male-female Dupont cable from the breadboard to the knob, in the
  module's pin order, so it can sit apart from the board. The encoder pins can
  move to D18/D19/D23 if a board redesign makes the right side more convenient.
- An LDR divider sat on D34 briefly on 2026-09-25; removed again, see ROADMAP
  task 5.
- Free, non-strapping GPIOs for future modules: D4, D16, D17, D18, D19, D23,
  D32, D33 (plus the input-only D34/D35/VP/VN).
