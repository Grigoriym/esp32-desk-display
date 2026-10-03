// Desk display enclosure, v3 (ROADMAP task 15): v2's `whole` layout with
// the ESP32 DevKit swapped for the ESP32 mini (D1 mini layout, 31.5 x 39 mm,
// decided 2026-10-03). The 4 x 6 cm perfboard stays uncut, the mini lies
// left-to-right with its USB-C out the right side, plugged into four 10-pin
// female headers. Its antenna end points left, over the carrier, ~5 mm
// from the nearest socket; all five JST-XH sockets sit in a stack at the
// carrier's left end. DS3231 on the floor under the carrier, as in v2.
// enclosure_v2.scad (DevKit) is kept for comparison.
// Sizes in mm: modules from their Homebox entries (parts.py show <part>),
// see MEASUREMENTS.md; values marked ASSUMED were not measured.
//
// Axes: X = width, left to right seen from the front; Y = depth, front at
// y = 0; Z = up, table at z = 0 (rubber feet not modelled).
//
// Printed parts:
//   shell - walls, tilted screen panel, top with the knob; open at the bottom
//   base  - floor plate: carrier board standoffs, sensor pockets, 4 screws
//           up into the shell's corner bosses
//   hood  - roof + back wall of the sensor bay, sits on the base
//   clamp - bar holding the OLED against the screen panel, print 2
// Pick one with -D 'part="shell"' (see export.sh). Single parts come out in
// print orientation, "assembly" shows everything in place with stand-in
// blocks for the modules, "clash" is empty when nothing overlaps.

part = "assembly"; // [assembly, shell, base, hood, clamp, test_front, clash]
cut = -1; // assembly only: >= 0 cuts the printed parts away left of this x
show_shell = true; // assembly only: untick to see inside
show_base = true;
show_hood = true;
show_clamps = true;
show_labels = true; // names over the module stand-ins

$fn = 48;
eps = 0.01;

// ------------------------------------------------------------ print
clr = 0.3; // fit clearance between printed parts / around boards

// ------------------------------------------------------------ modules
oled_w = 26.00;
oled_h = 26.04; // pin edge at the top
oled_t = 2.64; // incl. glass
oled_pcb_t = 1.0; // ASSUMED, only used for the stand-in: rest of oled_t is glass
oled_lit_w = 21.93;
oled_lit_h = 11.32;
oled_lit_top = 3.0; // lit area's top edge below the pin edge (+-0.5)
oled_back = 8.25; // header pins + plastic behind the PCB
oled_dupont = 14; // a plugged-in Dupont connector adds this

// ESP32 mini, in its own frame: x across the pin rows (0..esp_w), y along
// them, antenna end at y 0, USB-C end at esp_l
esp_l = 39.02;
esp_w = 31.51;
esp_t = 1.49;
esp_top = 4.66; // board bottom to the top of the metal can
esp_usb_z = 4.61 - 3.12 / 2; // USB-C centre above the board bottom
esp_usb_out = 1.22; // USB-C shell past the board edge
esp_pins = 10; // pads per line
esp_pin_x = [(esp_w - 27.94) / 2, (esp_w - 22.86) / 2, (esp_w + 22.86) / 2, (esp_w + 27.94) / 2]; // outer, inner, inner, outer
esp_pin_edge = 7.33; // antenna edge to the first pad's centre (6.83 to its near side + ~0.5)
esp_antenna = 6; // antenna end: keep clear

ky_l = 26.18;
ky_w = 19.29;
ky_t = 1.6; // ASSUMED
ky_cap_d = 14.78;
ky_shaft_y = 16 - ky_cap_d / 2; // shaft centre from the board's short edge away
                                // from the pins: that edge to the cap's far side
                                // was ~16 (2026-09-28, skewed reading, +-1)
ky_cap_h = 31.79 - 15.58; // cap height
ky_cap_above = 15.58 - ky_t; // board front face to the cap's lower edge
ky_body = 12.5; // EC11 body footprint (ASSUMED, standard part)
ky_body_h = 7; // EC11 body height above the board (ASSUMED)

ds_l = 31.93;
ds_w = 21.79;
ds_h = 9.39;
ds_pins6 = 14; // 6-pin end: right-angle pins + cable soldered on, bent away (ASSUMED)
ds_pins4 = 8; // pass-through 4-pin end: right-angle pins (ASSUMED)

bme_l = 13.15;
bme_w = 10.66;
bme_h = 3;

scd_l = 21.81;
scd_w = 13.49;
scd_h = 7.59;

// carrier board: the Miuzei 4 x 6 cm perfboard (14 x 20 holes), holes
// drilled to match the base's standoffs (lay it on the printed base, mark
// through, drill 2 mm). perf_w = size along X, perf_d = along Y
whole = true; // v2's naming: board uncut, USB-C out the right side
perf_w = 60;
perf_d = 40;
perf_t = 1.6;
perf_standoff = 16; // the DS3231 lies on the floor under it
perf_hole_inset = 3.5;
hdr_h = 8.5; // female header the ESP32 plugs into (ASSUMED: standard)

// ------------------------------------------------------------ case
wall = 2.0;
tilt = 20; // screen panel, back from vertical
skirt_h = 22; // vented strip under the screen, in front of the sensor bay
panel_margin = 4; // screen panel beyond the OLED board, top and bottom
W = 80;
D = 84;
base_t = 3;

panel_len = oled_h + 2 * panel_margin; // along the slope
H = skirt_h + panel_len * cos(tilt);
run = panel_len * sin(tilt); // how far back the panel's top edge is

boss_d = 7;
boss_pilot = 2.5; // M3 self-tapping, 10 deep
boss_in = wall + boss_d / 2 - 1; // centre from the outside; sunk 1 mm into both walls
boss_xy = [[boss_in, boss_in], [W - boss_in, boss_in], [boss_in, D - boss_in], [W - boss_in, D - boss_in]];

// sensor bay: y wall..bay_back, closed by the hood's roof at hood_top
bay_back = 22;
hood_t = 1.6;
hood_top = skirt_h - 1;
bme_xy = [24, 12];
scd_xy = [52, 12];
ledge_h = 1.5; // sensors sit on corner ledges, air flows underneath
rim = 1.2;

// OLED: glass flat against the panel's inner face, located by 4 corner
// stops, pressed on by two printed clamp bars (part "clamp", print 2)
// screwed to posts beside the board. Its corner holes aren't used: the
// glass reaches almost to them, no room for a screw post
oled_v = panel_len / 2; // board centre, up the slope from the skirt's edge
oled_post_x = oled_w / 2 + clr + 2.5; // clamp posts, left and right of the board
oled_post_d = 5;
oled_pilot = 1.6; // M2 x 5 self-tapping
clamp_t = 2;
clamp_overlap = 3; // how far a bar reaches over the board's back, along its side edge
lit_dz = oled_h / 2 - oled_lit_top - oled_lit_h / 2; // lit centre above board centre
win_w = oled_lit_w + 1;
win_h = oled_lit_h + 1;

// knob on top, left: KY-040 board flat under the top, pins to the back,
// held by two snap hooks on its long edges, pushed up against 4 pads
knob_x = 16;
ky_y0 = 16; // board's front edge
knob_y = ky_y0 + ky_shaft_y;
knob_gap = 1; // top surface to the cap's lower edge
knob_hole = 16;
ky_face_z = H + knob_gap - ky_cap_above; // board front (component) face
ky_back_z = ky_face_z - ky_t;

// Everything on the carrier sits on its 2.54 hole grid: hole (col, row) is
// at [grid_x0 + col * p, grid_y0 + row * p]. The board has 14 x 20 holes,
// ASSUMED centred (margins 3.49 across 14, 5.87 along 20).
p = 2.54;
usb_gap = 1.5; // USB-C shell to the wall
// 20 cols along X, 14 rows across Y. The mini's four pad lines run along X
// in rows 1, 2, 11, 12, pads in cols 10-19, USB end hanging ~3 mm off the
// right edge. Cols 0-4 hold the sockets; the antenna starts at col ~7
esp_x0 = W - wall - usb_gap - esp_usb_out - esp_l;
grid_x0 = esp_x0 + esp_pin_edge - 10 * p;
grid_y0 = D - wall - boss_d - 1 - 40 + 3.49;
perf_x0 = grid_x0 - 5.87;
perf_y0 = grid_y0 - 3.49;
perf_x1 = perf_x0 + perf_w;
perf_y1 = perf_y0 + perf_d;
perf_z = base_t + perf_standoff;
esp_y1 = grid_y0 + 12 * p + esp_pin_x[0]; // outer line nearest the back in row 12
esp_y0 = esp_y1 - esp_w;
esp_x1 = esp_x0 + esp_l;
esp_z = perf_z + perf_t + hdr_h; // mini board bottom
usb_x = (esp_x0 + esp_x1) / 2;
usb_y = (esp_y0 + esp_y1) / 2; // hole in the right wall (socket ~0.25 off centre: ignored)
usb_z = esp_z + esp_usb_z;
usb_hole = [13, 8];

// DS3231, on the floor in a holder, coin cell up. Box ds_b = [x0, y0, x1, y1]
// is the board; its pins stick out of both short ends. cut: long axis
// front-to-back, 6-pin end to the front. whole: long axis left-to-right
// under the carrier, 6-pin end to the left
ds_z = base_t + ledge_h;
ds_b = whole ? [26, (perf_y0 + perf_y1) / 2 - ds_w / 2, 26 + ds_l, (perf_y0 + perf_y1) / 2 + ds_w / 2]
             : [perf_x0 - rim - clr - 1 - ds_w, 39, perf_x0 - rim - clr - 1, 39 + ds_l];

echo(str("case ", W, " x ", D, " x ", H, " mm (W x D x H)"));
echo(str("usb centre z ", usb_z, ", knob board face z ", ky_face_z));
assert(perf_x1 < W - wall && perf_y1 < D - wall, "carrier hits a wall");
assert(esp_x1 + esp_usb_out < W - wall && esp_y1 < D - wall, "ESP32 hits a wall");

// JST-XH sockets (vertical, 2.5 pitch) on the carrier: [name, pins, col,
// row of the first pin, pin line along X?]. One stack at the left end, pin
// lines along X, 3 rows apart (plugs 6.4 wide)
xh = [["BME280", 4, 0, 0, true], ["SCD41", 4, 0, 3, true], ["OLED", 4, 0, 6, true],
      ["RTC", 4, 0, 9, true], ["KNOB", 5, 0, 12, true]];
xh_missing = "";
xh_h = 7; // socket height
xh_plug = 12; // mated plug + cable bend above the board (ASSUMED)
for (c = xh) let(l = (c[1] - 1) * 2.5 + 4.9, at = hole(c[2], c[3]), e = [at.x - 2.9, at.y - 2.9,
  at.x + (c[4] ? (c[1] - 1) * p : 0) + 2.9, at.y + (c[4] ? 0 : (c[1] - 1) * p) + 2.9])
  assert(e[0] > perf_x0 && e[1] > perf_y0 && e[2] < perf_x1 && e[3] < perf_y1, str(c[0], " socket off the carrier"));
if (xh_missing != "") echo(str("no room on the carrier for sockets: ", xh_missing));
assert(!whole || perf_z - 3 > ds_z + ds_h, "no room for the DS3231 under the carrier");

// ------------------------------------------------------------ helpers
module profile() polygon([[0, 0], [0, skirt_h], [run, H], [D, H], [D, 0]]);

module extrude_x(x0, w) translate([x0, 0, 0]) rotate([90, 0, 90]) linear_extrude(w) children();

module outer() extrude_x(0, W) profile();

// inside of the shell, open at the bottom
module cavity() extrude_x(wall, W - 2 * wall) hull() {
  offset(delta = -wall) profile();
  translate([0, -wall - 1]) offset(delta = -wall) profile();
}

// panel coordinates: x across (0 = centre), y inwards (0 = outer face),
// z up the slope (0 = the skirt's top edge)
module panel_frame() translate([W / 2, 0, skirt_h]) rotate([-tilt, 0, 0]) children();

// at the OLED board's centre, on the panel's inner face
module oled_frame() panel_frame() translate([0, wall, oled_v]) children();

module box(p0, p1) translate(p0) cube(p1 - p0);

module slots(x0, x1, y0, y1, z0, z1, pitch = 5, w = 2) {
  n = floor((x1 - x0 - w) / pitch);
  for (i = [0:n]) box([x0 + i * pitch, y0, z0], [x0 + i * pitch + w, y1, z1]);
}

module stadium(w, h, len) // along y, centred in x/z
  hull() for (s = [-1, 1]) translate([s * (w - h) / 2, 0, 0]) rotate([-90, 0, 0]) cylinder(d = h, h = len);

// ------------------------------------------------------------ shell
module oled_mount() oled_frame() {
  for (sx = [-1, 1]) translate([sx * oled_post_x, -0.5, 0]) rotate([-90, 0, 0]) cylinder(d = oled_post_d, h = oled_t + 0.5);
  // L-shaped corner stops; the flex cable passes between the bottom two
  for (sx = [-1, 1], sz = [-1, 1]) {
    ex = oled_w / 2 + clr;
    ez = oled_h / 2 + clr;
    box([min(sx * ex, sx * (ex + 1.2)), -0.5, min(sz * (ez - 4), sz * (ez + 1.2))], [max(sx * ex, sx * (ex + 1.2)), 2, max(sz * (ez - 4), sz * (ez + 1.2))]);
    box([min(sx * (ex - 4), sx * (ex + 1.2)), -0.5, min(sz * ez, sz * (ez + 1.2))], [max(sx * (ex - 4), sx * (ex + 1.2)), 2, max(sz * ez, sz * (ez + 1.2))]);
  }
}

module oled_pilots() oled_frame() for (sx = [-1, 1])
  translate([sx * oled_post_x, oled_t + eps, 0]) rotate([90, 0, 0]) cylinder(d = oled_pilot, h = oled_t + wall - 0.8);

// clamp bar, in oled_frame coordinates for the right side (mirrored for the left)
module clamp_bar() difference() {
  box([oled_w / 2 - clamp_overlap, oled_t, -oled_h / 2 + 1], [oled_post_x + oled_post_d / 2, oled_t + clamp_t, oled_h / 2 - 1]);
  translate([oled_post_x, oled_t - 1, 0]) rotate([-90, 0, 0]) cylinder(d = 2.4, h = clamp_t + 2);
}

// lit area plus 0.5 all round, chamfered 45 degrees outwards
module oled_window() oled_frame() translate([0, 0, lit_dz]) hull() {
  translate([0, 0.5, 0]) cube([win_w, 1, win_h], center = true);
  translate([0, -wall - 0.5, 0]) cube([win_w + 2 * (wall + 1), 1, win_h + 2 * (wall + 1)], center = true);
}

module bosses() for (p = boss_xy) translate([p.x, p.y, base_t]) cylinder(d = boss_d, h = H);

module boss_pilots() for (p = boss_xy) translate([p.x, p.y, base_t - 1]) cylinder(d = boss_pilot, h = 11);

module knob_mount() {
  top_in = H - wall + 0.5; // reach into the top wall
  // pads the board is pushed against, near its corners, clear of the EC11
  for (sx = [-1, 1], y = [ky_y0 + 2.5, ky_y0 + ky_l - 4])
    translate([knob_x + sx * (ky_w / 2 - 1.8), y, ky_face_z + eps]) cylinder(d = 3, h = top_in - ky_face_z);
  // snap hooks on the long edges, beside the shaft
  for (sx = [-1, 1]) {
    edge = knob_x + sx * (ky_w / 2 + 0.2);
    box([min(edge, edge + sx * 1.4), knob_y - 3, ky_back_z - 1.8], [max(edge, edge + sx * 1.4), knob_y + 3, top_in]);
    // barb: flat face under the board, ramp below for pushing it in
    hull() {
      box([min(edge, edge - sx * 1.0), knob_y - 3, ky_back_z - 0.9], [max(edge, edge - sx * 1.0), knob_y + 3, ky_back_z - 0.15]);
      box([min(edge, edge + sx * 0.5), knob_y - 3, ky_back_z - 1.8], [max(edge, edge + sx * 0.5), knob_y + 3, ky_back_z - 1.7]);
    }
  }
}

module vents() {
  // front skirt, in front of the sensors
  slots(14, W - 14, -1, wall + 1, 6, skirt_h - 5);
  // bay sides, behind the front bosses
  for (x = [-1, W - wall - 1]) for (z = [6:4:skirt_h - 5]) box([x, 11, z], [x + wall + 2, bay_back - 2, z + 2]);
  // top, over the ESP32: its warm air leaves here
  slots(esp_x0 + 2, esp_x1 - 2, whole ? esp_y0 + 2 : esp_y0 + 14, whole ? esp_y1 - 2 : D - wall - 4, H - wall - 1, H + 1);
  // back, low: intake (left of the USB when it's there)
  slots(14, whole ? W - 14 : usb_x - 12, D - wall - 1, D + 1, base_t + 2, base_t + 9);
}

module shell() difference() {
  union() {
    difference() {
      outer();
      cavity();
    }
    intersection() {
      outer();
      union() {
        bosses();
        oled_mount();
        knob_mount();
      }
    }
  }
  boss_pilots();
  oled_window();
  oled_pilots();
  vents();
  if (whole) translate([W - wall - 1, usb_y, usb_z]) rotate([0, 0, -90]) stadium(usb_hole[0], usb_hole[1], wall + 2);
  else translate([usb_x, D - wall - 1, usb_z]) stadium(usb_hole[0], usb_hole[1], wall + 2);
  translate([knob_x, knob_y, H - wall - 1]) cylinder(d = knob_hole, h = wall + 2);
}

// ------------------------------------------------------------ base
module pocket(xy, l, w) translate([xy.x, xy.y, base_t - eps]) {
  ix = l / 2 + clr;
  iy = w / 2 + clr;
  h = ledge_h + 1.6 + 1;
  difference() {
    box([-ix - rim, -iy - rim, 0], [ix + rim, iy + rim, h]);
    box([-ix, -iy, -1], [ix, iy, h + 1]);
    box([-4, 0, ledge_h], [4, iy + rim + 1, h + 1]); // cable exit, towards the hood's notch
  }
  for (sx = [-1, 1], sy = [-1, 1]) translate([sx * (ix - 1.25), sy * (iy - 1.25), 0]) cube([2.5, 2.5, 2 * ledge_h], center = true);
}

// DS3231 local coordinates: board x 0..ds_w, y 0..ds_l (6-pin end at y < 0),
// z 0 = board bottom; placed in ds_b
module ds_place() if (whole) translate([ds_b[0], ds_b[3], 0]) rotate([0, 0, -90]) children();
                  else translate([ds_b[0], ds_b[1], 0]) children();

// rails along the long edges and 4 corner ledges; the ends stay open for the pins
module ds_holder() ds_place() translate([0, 0, base_t - eps]) {
  h = ledge_h + 1.6 + 1;
  for (x = [-clr - rim, ds_w + clr]) box([x, 2, 0], [x + rim, ds_l - 2, h]);
  for (x = [0, ds_w - 2.5], y = [0, ds_l - 2.5]) box([x, y, 0], [x + 2.5, y + 2.5, ledge_h]);
}

perf_holes = [for (x = [perf_x0 + perf_hole_inset, perf_x0 + perf_w - perf_hole_inset])
    for (y = [perf_y0 + perf_hole_inset, perf_y1 - perf_hole_inset]) [x, y]];

module base() {
  x0 = wall + clr;
  y0 = wall + clr;
  difference() {
    box([x0, y0, 0], [W - x0, D - y0, base_t]);
    for (p = boss_xy) translate([p.x, p.y, -1]) {
      cylinder(d = 3.4, h = base_t + 2);
      cylinder(d = 6.5, h = 1 + 2); // M3 head counterbore, 1 mm left
    }
    slots(12, W - 12, 5, bay_back - 3, -1, base_t + 1); // under the sensors
    slots(12, W - 12, bay_back + hood_t + 3, min(perf_y0, ds_b[1] - ds_pins6) - 1, -1, base_t + 1); // intake in front of the carrier
  }
  pocket(bme_xy, bme_l, bme_w);
  pocket(scd_xy, scd_l, scd_w);
  ds_holder();
  for (p = perf_holes) translate([p.x, p.y, base_t - eps]) difference() {
    cylinder(d = 6, h = perf_standoff);
    cylinder(d = 1.7, h = perf_standoff + 1); // M2 x 8 self-tapping
  }
  // two ribs the hood's back wall drops between, gap at the cable notch
  for (y = [bay_back - clr - 1.2, bay_back + hood_t + clr]) for (xs = [[x0 + 10, W / 2 - 9], [W / 2 + 9, W - x0 - 10]])
    box([xs[0], y, base_t - eps], [xs[1], y + 1.2, base_t + 2]);
}

// ------------------------------------------------------------ hood
module hood() difference() {
  x0 = wall + clr;
  union() {
    box([x0, bay_back, base_t], [W - x0, bay_back + hood_t, hood_top]);
    box([x0, wall + clr, hood_top - hood_t], [W - x0, bay_back + hood_t, hood_top]);
  }
  for (p = boss_xy) translate([p.x - boss_d / 2 - 0.5, p.y - boss_d / 2 - 0.5, 0]) cube([boss_d + 1, boss_d + 1, H]);
  box([W / 2 - 8, bay_back - 1, base_t - 1], [W / 2 + 8, bay_back + hood_t + 1, base_t + 6]); // sensor cables
}

// ------------------------------------------------------------ stand-ins
// ESP32 local coordinates: x 0..esp_w, y 0..esp_l (antenna at y 0, USB at
// esp_l), world z; placed in the esp box
// one JST-XH socket, with its mated plug + cable bend as a see-through block
function hole(col, row) = [grid_x0 + col * p, grid_y0 + row * p];

module xh_frame(c) {
  n = c[1];
  mid = (n - 1) * p / 2;
  at = hole(c[2], c[3]);
  translate([at.x, at.y, perf_z + perf_t]) if (c[4]) translate([mid, 0, 0]) children();
  else translate([0, mid, 0]) rotate([0, 0, 90]) children();
}

module xh_socket(c) xh_frame(c) {
  l = (c[1] - 1) * 2.5 + 4.9;
  color("white") box([-l / 2, -5.75 / 2, eps], [l / 2, 5.75 / 2, xh_h]);
  color("gold", 0.35) box([-l / 2 - 0.3, -3.2, xh_h + eps], [l / 2 + 0.3, 3.2, xh_plug]);
}

module xh_label(c) xh_frame(c) color("black") translate([0, 0, xh_plug + 0.3])
  linear_extrude(0.3) text(c[0], size = 2, halign = "center", valign = "center");

module esp_place() if (whole) translate([esp_x0, esp_y1, 0]) rotate([0, 0, -90]) children();
                   else translate([esp_x0, esp_y0, 0]) children();

module stand_ins() {
  color("steelblue") oled_frame() {
    box([-oled_w / 2, oled_t - oled_pcb_t, -oled_h / 2], [oled_w / 2, oled_t - eps, oled_h / 2]);
    translate([0, 0, lit_dz]) box([-oled_w / 2, eps, -8], [oled_w / 2, oled_t - oled_pcb_t, 8]);
    box([-5.1, oled_t + eps, oled_h / 2 - 2.54], [5.1, oled_t + oled_back, oled_h / 2]);
    %box([-5.1, oled_t + oled_back, oled_h / 2 - 2.54], [5.1, oled_t + oled_back + oled_dupont, oled_h / 2]);
  }
  color("darkgreen") box([perf_x0, perf_y0, perf_z], [perf_x0 + perf_w, perf_y1, perf_z + perf_t]);
  esp_place() {
    color("dimgray") for (x = esp_pin_x) // female headers
      box([x - p / 2, esp_pin_edge - p / 2, perf_z + perf_t], [x + p / 2, esp_pin_edge + (esp_pins - 0.5) * p, esp_z]);
    color("navy") box([0, 0, esp_z], [esp_w, esp_l, esp_z + esp_t]);
    color("silver") box([esp_w / 2 - 9, esp_antenna + 1, esp_z + esp_t], [esp_w / 2 + 9, 25, esp_z + esp_top]); // can
    color("silver") box([esp_w / 2 - 4.45, esp_l - 7, esp_z + esp_t], [esp_w / 2 + 4.45, esp_l + esp_usb_out, esp_z + 4.61]); // USB-C
    color("orange", 0.5) box([0, 0, esp_z + esp_t], [esp_w, esp_antenna, esp_z + esp_top]); // antenna: keep clear
  }
  for (c = xh) xh_socket(c);
  ds_place() {
    color("navy") box([0, 0, ds_z], [ds_w, ds_l, ds_z + ds_h]);
    color("dimgray", 0.6) {
      box([ds_w / 2 - 7.6, -ds_pins6, ds_z + 1.6], [ds_w / 2 + 7.6, -eps, ds_z + 4.1]); // 6-pin + cable
      box([ds_w / 2 - 5.1, ds_l + eps, ds_z + 1.6], [ds_w / 2 + 5.1, ds_l + ds_pins4, ds_z + 4.1]); // 4-pin
    }
  }
  color("purple") {
    box([bme_xy.x - bme_l / 2, bme_xy.y - bme_w / 2, base_t + ledge_h], [bme_xy.x + bme_l / 2, bme_xy.y + bme_w / 2, base_t + ledge_h + bme_h]);
    box([scd_xy.x - scd_l / 2, scd_xy.y - scd_w / 2, base_t + ledge_h], [scd_xy.x + scd_l / 2, scd_xy.y + scd_w / 2, base_t + ledge_h + scd_h]);
  }
  color("firebrick") {
    box([knob_x - ky_w / 2, ky_y0, ky_back_z], [knob_x + ky_w / 2, ky_y0 + ky_l, ky_face_z]);
    translate([knob_x, knob_y, ky_face_z + ky_body_h / 2]) cube([ky_body, ky_body, ky_body_h], center = true);
  }
  color("gray") translate([knob_x, knob_y, H + knob_gap]) cylinder(d = ky_cap_d, h = ky_cap_h);
}

// a name on a stick from point p, the text at dz above the case top,
// facing the front
module label(txt, p, dz) color("black") translate(p) {
  cylinder(d = 0.4, h = H + dz - p.z, $fn = 6);
  translate([0, 0, H + dz - p.z + 0.5]) rotate([90, 0, 0]) linear_extrude(0.4) text(txt, size = 2.5, halign = "center");
}

// panel coordinates (x, inwards, up the slope) to world
function panel_pt(x, y, z) = [W / 2 + x, y * cos(tilt) + z * sin(tilt), skirt_h + z * cos(tilt) - y * sin(tilt)];

module labels() {
  for (c = xh) xh_label(c);
  label("OLED", panel_pt(0, wall + oled_t + oled_back, oled_v + oled_h / 2), 13);
  label("ESP32 mini", [usb_x, usb_y, esp_z + esp_top], 3);
  label("antenna", whole ? [esp_x0 + 3, usb_y, esp_z + esp_top] : [usb_x, esp_y0 + 3, esp_z + esp_top], 13);
  label("USB-C", whole ? [esp_x1, usb_y, esp_z + esp_top] : [usb_x, esp_y1, esp_z + esp_top], 18);
  label("carrier", whole ? [perf_x0 + 6, perf_y0 + 3, perf_z + perf_t] : [perf_x0 + 2, perf_y1 - 3, perf_z + perf_t], 8);
  label("DS3231", [(ds_b[0] + ds_b[2]) / 2, (ds_b[1] + ds_b[3]) / 2, ds_z + ds_h], 25);
  label("KY-040", [knob_x, knob_y, H + knob_gap + ky_cap_h], 20);
  label("BME280", [bme_xy.x, bme_xy.y, base_t + ledge_h + bme_h], 8);
  label("SCD41", [scd_xy.x, scd_xy.y, base_t + ledge_h + scd_h], 8);
}

// ------------------------------------------------------------ output
module test_front() intersection() {
  shell();
  box([-1, -1, skirt_h - 3], [W + 1, ky_y0 + ky_l + 3, H + 1]);
}

if (part == "shell") translate([0, D, H]) rotate([180, 0, 0]) shell();
else if (part == "test_front") translate([0, D, H]) rotate([180, 0, 0]) test_front();
else if (part == "base") base();
else if (part == "clamp") for (i = [0, 1]) translate([i * 12, 0, 0])
  translate([0, 0, -oled_t]) rotate([90, 0, 0]) clamp_bar();
else if (part == "hood") translate([0, 0, hood_top]) rotate([180, 0, 0]) hood();
else if (part == "dims") echo(W = W, D = D, H = H, wall = wall, tilt = tilt, skirt_h = skirt_h, run = run,
  panel_len = panel_len, oled_w = oled_w, oled_h = oled_h, oled_v = oled_v, lit_dz = lit_dz, win_w = win_w,
  win_h = win_h, knob_x = knob_x, knob_y = knob_y, knob_hole = knob_hole, ky_y0 = ky_y0, ky_l = ky_l, ky_w = ky_w,
  usb_x = usb_x, usb_y = usb_y, usb_z = usb_z, usb_w = usb_hole[0], usb_h = usb_hole[1], usb_wall = whole ? 1 : 0, boss_in = boss_in, boss_d = boss_d,
  bay_back = bay_back, hood_t = hood_t, bme_x = bme_xy.x, bme_y = bme_xy.y, bme_l = bme_l, bme_w = bme_w,
  scd_x = scd_xy.x, scd_y = scd_xy.y, scd_l = scd_l, scd_w = scd_w, perf_x0 = perf_x0, perf_y0 = perf_y0,
  perf_w = perf_w, perf_d = perf_d, esp_x0 = esp_x0, esp_y0 = esp_y0, esp_w = esp_x1 - esp_x0, esp_l = esp_y1 - esp_y0,
  esp_antenna = esp_antenna, ant_x = whole ? esp_x0 + 1 : esp_x0 + 1, ant_y = whole ? esp_y0 - 1.5 : esp_y0 + 2,
  ds_x0 = ds_b[0], ds_y0 = ds_b[1], ds_w = ds_b[2] - ds_b[0], ds_l = ds_b[3] - ds_b[1], ds_under = whole ? 1 : 0); // for cardboard.py
else if (part == "clash") {
  intersection() {
    union() {
      shell();
      base();
      hood();
      oled_frame() for (m = [0, 1]) mirror([m, 0, 0]) clamp_bar();
    }
    stand_ins();
  }
  // sockets and plugs against the ESP32 (and its headers) and the DS3231
  intersection() {
    for (c = xh) xh_socket(c);
    union() {
      esp_place() {
        box([0, 0, esp_z], [esp_w, esp_l + esp_usb_out, esp_z + esp_top]);
        for (x = esp_pin_x) box([x - p / 2, esp_pin_edge - p / 2, perf_z + perf_t], [x + p / 2, esp_pin_edge + (esp_pins - 0.5) * p, esp_z]);
      }
      ds_place() box([0, -ds_pins6, ds_z], [ds_w, ds_l + ds_pins4, ds_z + ds_h]);
    }
  }
}
else {
  if (show_clamps) color("dimgray") oled_frame() for (m = [0, 1]) mirror([m, 0, 0]) clamp_bar();
  intersection() {
    union() {
      if (show_shell) color("white") shell();
      if (show_base) color("khaki") base();
      if (show_hood) color("tan") hood();
    }
    if (cut >= 0) box([cut, -10, -10], [W + 10, D + 10, H + 10]);
    else box([-10, -10, -10], [W + 10, D + 10, H + 10]);
  }
  stand_ins();
  if (show_labels) labels();
}
