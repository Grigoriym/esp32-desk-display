# Part measurements (ROADMAP task 15)

Step 1 of the enclosure: the user measures every part with a digital
caliper (ordered 2026-09-27) and fills in the blanks below, in mm. The
OpenSCAD model is written from these numbers, not from datasheet guesses.
"From the left/top edge": hold the board with its pins at the top.

## OLED (SSD1315, 0.96")
- Board width x height x thickness (incl. glass): 26.00 x 26.04 x 2.64
- Lit area: size 21.93 x 11.32 (w x h); position estimated from a straight-on photo (2026-09-28, +-0.5): top of the lit area ~3 mm below the top (pin-side) edge, roughly centred left-right; confirm with the front-panel test print
- Mounting holes: 4, one per corner (seen in the photos), diameter ~1.70 measured (tiny hole, inside jaws read low: likely 2.0 nominal, for M2 screws), centre spacing 22.00 left-right x 21.52 top-bottom, so hole centres ~2.0 mm from the left/right edges and ~2.26 mm from the top/bottom edges
- Header pins stick out at the back: 8.25 from the back surface, incl. the black plastic (a plugged-in Dupont connector adds ~14 more: leave room behind the OLED)
- From photos (2026-09-28): board is JMD0.96D-1; 4-pin header (GND VCC SCL SDA) soldered with its black plastic on the **back**; the display's orange flex cable wraps around the **bottom** edge through a notch, and the back has small SMD parts: the case must not press on the flex or the back

## ESP32 DevKit (ELEGOO ESP-32S, 30-pin)
- Board length x width: 51.49 x 28.36 (measured; the seller's drawing says 51.74 x 29, trust ours)
- Pin rows, center to center: 25.4 (10 x 2.54 pitch; seller's drawing says 25.64 = 1.01", the rows must sit on the 2.54 perfboard grid anyway), pins 2.54 apart, 15 per row
- From the seller's images (2026-09-28): 4 mounting holes, one per corner; pins stick out ~6 below the board; the USB-C port overhangs the short edge
- USB-C port: on the short edge opposite the antenna, standard socket ~8.9 x 3.2; board bottom to port top 4.75, so it sits on top of the board (1.6 board assumed + ~3.15 port), port centre ~3.2 above the board bottom
- Tallest part on top (metal shield): ___
- Antenna end: opposite the USB (seen in the seller's image: antenna on the module's top end, above the metal shield)

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
