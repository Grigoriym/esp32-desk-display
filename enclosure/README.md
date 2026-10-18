# Enclosure (ROADMAP task 15)

OpenSCAD model of the case, `enclosure.scad`, for the first finished build:
ESP32 DevKit, OLED, KY-040, SCD41, BME280, DS3231, no battery. Open it in
OpenSCAD for the assembled view (modules as coloured stand-in blocks,
see-through gold = plugged-in connectors). `-D cut=40` cuts the printed
parts away left of x = 40; `-D explode=40 -D explode_front=30` pulls the
parts apart.

Part dimensions and how each part can be held: its Homebox entry
(`python3 ../../grappim-watcher/docs/esp32/inventory/parts.py show <part>`).
General modelling, clash-check and printability lessons:
`../../grappim-watcher/docs/esp32/ENCLOSURE_PLAYBOOK.md`. This file is this
case only. Earlier models (v1-v3: the ESP32 across the 4 cm side, the cut
carrier, the ESP32 mini) are in git history before 2026-10-18.

```
enclosure/export.sh      # checks, then stl/enclosure/{body,front,base,hood,clamp,test_knob}.stl (gitignored) + renders/
enclosure/cardboard.py   # 1:1 A4 templates -> enclosure/cardboard.pdf (gitignored)
```

`export.sh` fails if a printed part overlaps a stand-in or another printed
part, if two stand-ins (plugs included) overlap, or if a pre-print assert
in the model fails (plastic round holes, pilot depth vs screw reach, gaps,
thread above the knob nut). It prints those margins (`CHECKS:`) and every
`ASSUMED` value.

![exploded](renders/enclosure-exploded.png) ![inside](renders/enclosure-inside.png)
![front](renders/enclosure-front.png) ![back](renders/enclosure-back.png)

## Layout

80 x 84 x 54 mm (W x D x H). A wedge: the OLED on a panel tilted 20° back,
a vented strip under it, the knob on top (left), USB-C out the right side.

Five printed parts:
- **body**: side walls, top, back; open at the bottom and the front.
- **front plate**: the tilted screen panel, dropped in between the side
  walls onto a rail along each, 4 screws from the front (heads visible).
  Its own part so the OLED's clamp screws can be driven with the plate on
  the desk; a wrong window costs one small plate.
- **base**: floor over the whole footprint (the body's walls stand on it),
  with the vented strip under the screen, sensor pockets, the carrier's
  standoffs and the DS3231's holder. 4 × M3 from below into the body's
  corner bosses, so the case opens without touching the wiring.
- **hood**: roof + back wall of the sensor bay, sits on the base between
  two ribs.
- **clamp** × 2: bars that press the OLED onto the plate.

Inside:
- **Sensor bay** (BME280, SCD41): low at the front, walled off by the hood.
  Air in through the floor slots, out through the strip and the side
  slots. Nothing warm inside it. Cables out through a notch in the hood's
  back wall. ~16 mm tall inside: solder the sensor cables straight on or
  use right-angle pins.
- **Carrier**: the uncut 60 x 40 Miuzei perfboard on 4 standoffs, 16 mm
  tall: the DS3231 lies on the floor under it (holder, coin cell up, JST
  cable). The ESP32 plugs into two 15-pin female headers on it, left to
  right, USB-C out the right side; its antenna end is over the board, under
  the knob, nothing else above it. Five JST-XH sockets: BME280, SCD41, OLED
  along the front edge, RTC and KNOB at the left end. The hole grid is
  ASSUMED centred on the board.
- **OLED**: glass flat on the plate's inner face between 4 corner stops,
  two clamp bars screwed to posts beside it. The posts stop 0.3 short of
  the PCB's back, so the bars press on it. Its corner holes aren't used
  (the glass reaches almost to them). Pins at the top. Upside-down picture
  → flip the two orientation bytes in `display.c`, not the board.
- **Knob**: the KY-040 flat under the top, pins (right-angle header) to the
  back, its plug pointing at the carrier's KNOB socket. The M7 collar goes
  up through a 7.3 hole and its own nut + washer clamp it from outside; two
  ribs at the pin end stop it turning. Cap pushed on afterwards.
- **Vents**: top slots over the ESP32 (its warm air rises away from the
  sensors), low slots in the back, floor slots.

## Parts to print

| STL | Qty | Orientation (as exported) | Notes |
|---|---|---|---|
| `body` | 1 | upside down, top on the bed | check the USB hole's bridge |
| `front` | 1 | screen face on the bed | window chamfered, no supports |
| `base` | 1 | flat | |
| `hood` | 1 | roof on the bed | |
| `clamp` | 2 (file has both) | flat | |
| `test_knob` | optional | like the body | the top's front with the knob mount |

Material: PETG preferred (PLA softens ~55 °C). Fit clearance 0.3 (`clr`).

## Hardware

Self-tapping screws from the two kits (Homebox: `parts.py show screws`):
- 4 × M3 × 10 round head, silver kit (base → body; 3.64 × 11.7 measured:
  4.0 holes, pilot 2.5, 13 deep)
- 4 × 2 × 6, black kit (front plate → rails; pilot 1.6)
- 2 × 2 × 4, black kit (OLED clamps; pilot 1.6)
- 4 × 2 × 6, black kit (carrier → standoffs; pilot 1.6). Lay the perfboard
  in its outline on the base, mark the standoffs through, drill 2.5.
- 2 × 15-pin female headers, JST-XH sockets + cables, 4 rubber feet ≥ 2 mm
  (they lift the floor slots off the desk)

## Assembly

1. DS3231 into its holder, then the carrier onto the standoffs *before*
   the ESP32 goes into its headers (a driver above two of the holes hits
   the ESP32).
2. Sensors into their pockets, hood on.
3. Knob up through the top from inside, washer + nut from outside, cap on.
4. OLED onto the plate (plate on the desk), the two clamp bars.
5. Cables onto the carrier; plate onto the body (4 screws from the front);
   body down onto the base (4 screws from below).

## Before ordering

1. **Measure the female headers' height** (`hdr_h` = 8.5 assumed): it sets
   the ESP32's height and so the USB hole's (8 tall for a ~6.5 plug: ±0.75).
2. Where the perfboard's hole grid sits (assumed centred): shifts the USB
   sideways (hole 13 wide for a ~12 plug).
3. Check in OpenSCAD and against the cardboard mock-up.

## Printed-part checklist

1. **Front plate**: the OLED sits flat between the stops, the clamp bars
   screw down without bending it. The window is the lit area + 0.5 all
   round, lit area ±0.5 (from a photo): the time (flush left), the date's
   last digit (flush right) and the bottom row must show in full; else only
   the plate is reprinted. The plate drops in between the side walls and
   its holes meet the rails' pilots.
2. **Knob**: the collar passes the 7.3 hole (else drill 7.5), the board
   sits between the ribs, the nut tightens with thread to spare, the cap
   turns and presses without touching the nut.
3. **Base**: the strip sits 1.0 below the plate's bottom edge; the screws
   go in without forcing.
4. **USB**: a cable plugs in fully through the side hole.
5. Then compare the BME280 with a reference thermometer, case open vs
   closed.

## Cardboard mock-up

```
enclosure/cardboard.py [cardboard mm, default 2]
```

Two A4 pages of 1:1 templates, sized from the model: 2 sides, front strip,
screen panel (with the window), top (with the knob hole), back, and a floor
with the modules' outlines drawn on it (not holes) to lay the real ones on.
Print at 100% ("Actual size") and check the 50 mm bar with a ruler. Panels
other than the sides are narrower by 2 × the cardboard thickness, so they
fit between the sides. The mock-up checks size, screen angle, knob reach
and cable space; not the printed fits or the heat.
