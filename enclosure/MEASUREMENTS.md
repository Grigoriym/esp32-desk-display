# Part measurements (ROADMAP task 15)

Step 1 of the enclosure: the user measures every part with a digital
caliper (ordered 2026-09-27) and fills in the blanks below, in mm. The
OpenSCAD model is written from these numbers, not from datasheet guesses.
"From the left/top edge": hold the board with its pins at the top.

## OLED (SSD1315, 0.96")
- Board width x height x thickness (incl. glass): ___ x ___ x ___
- Lit area: size ___ x ___, from left edge ___, from top edge ___
- Mounting holes: diameter ___, centers from the board edges ___
- Header pins stick out at the back: ___

## ESP32 DevKit (ELEGOO ESP-32S, 30-pin)
- Board length x width: ___ x ___
- Pin rows, center to center: ___
- USB-C port: on which edge ___, width ___, height above the board ___
- Tallest part on top (metal shield): ___
- Antenna end (zigzag copper, opposite the USB): confirm ___

## KY-040 encoder
- Board length x width: ___ x ___, mounting holes (if any): ___
- Shaft diameter ___, length above the threaded collar ___
- Threaded collar: diameter ___, length ___ (limits the front panel thickness)
- Knob cap diameter: ___

## DS3231 RTC
- Length x width x tallest part (coin cell counts): ___ x ___ x ___
- Mounting holes: diameter ___, positions ___

## BME280
- Length x width x tallest part: ___ x ___ x ___
- Mounting holes: diameter ___, positions ___

## SCD41
- Length x width x tallest part: ___ x ___ x ___
- Mounting holes: diameter ___, positions ___

## Other
- USB-C cable plug housing: width ___ x height ___
- Wanted case size (rough): ___
- Wanted screen tilt: ___ (default ~20 degrees)
- Printer contact: material (PLA/PETG) ___, clearance they use ___ (usually 0.2-0.3)
