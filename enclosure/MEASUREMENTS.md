# Part measurements (ROADMAP task 15)

The caliper measurements of each part (in mm) live only in the part's
Homebox entry (fields "Board size", "Mounting holes", ...): read them with
`python3 ../../grappim-watcher/docs/esp32/inventory/parts.py show <part>`.
The OpenSCAD model is written from those numbers, not from datasheet
guesses. A new or corrected measurement goes in the entry, not here.

Measured 2026-09-28: OLED (SSD1315), ESP32 DevKit 30-pin, KY-040, DS3231,
BME280, SCD41. Measured 2026-10-03 (battery, not in the model yet): LiPo
103745, FM5324 charge module, USB-C breakout.

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
