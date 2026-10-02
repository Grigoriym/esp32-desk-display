# Part measurements (ROADMAP task 15)

The caliper measurements of each part (taken 2026-09-28, in mm) live in the
shared part sheets, `../../grappim-watcher/docs/esp32/parts/` (section
"Mechanical" of each). The OpenSCAD model is written from those numbers, not
from datasheet guesses. A new or corrected measurement goes in the sheet.

| Part | Sheet |
|---|---|
| OLED (SSD1315, 0.96") | `oled-ssd1315.md` |
| ESP32 DevKit (ELEGOO ESP-32S, 30-pin) | `esp32-devkit-30pin.md` |
| KY-040 encoder | `ky-040.md` |
| DS3231 RTC | `ds3231.md` |
| BME280 | `bme280.md` |
| SCD41 | `scd41.md` |

How to measure: `../../grappim-watcher/docs/esp32/ENCLOSURE_PLAYBOOK.md`.

## This case's choices
- KY-040 (2026-09-28): the knob **cap** sticks out through a round hole in the
  case and the board sits behind the panel, so shaft/collar sizes aren't needed.
  Top hole 16 mm.
- DS3231: its mounting holes aren't used.
- USB-C cable: an ordinary one, not measured. Rear hole **13 x 8, rounded ends**.
- Case size: as small as the parts allow (no preference given)
- Screen tilt: ~20 degrees (default)
- Printer material: ask the printing person; PETG preferred, clearance 0.2-0.3 assumed
- Battery: **not in the MVP** (2026-09-28). Planned later (see CLAUDE.md Open
  questions), so keep the case easy to open and don't pack it so tight a small
  cell can never fit
