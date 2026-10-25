// Desk display enclosure (ROADMAP task 15): ESP32 mini, OLED, KY-040,
// SCD41, BME280, DS3231, no battery. The uncut 4 x 6 cm perfboard is the
// carrier, socket side up: the mini plugged into it left-to-right, USB-C
// out the right side, the modules' pin headers beside it, the soldered
// wires underneath; DS3231 on the floor under the carrier (carrier built
// 2026-10-20, measured that day). Fits and fasteners follow what mukklet's printed
// cases taught (esp32-mukklet, 2026-10-07 and 2026-10-16;
// ENCLOSURE_PLAYBOOK.md):
// - the screen panel is its own part (front plate, 4 screws from the
//   front): in a one-piece shell no screwdriver reaches the OLED clamps'
//   screws (their axis runs into the back wall)
// - the strip under the screen is on the base: on the body it would be a
//   76 mm bridge when printed upside down
// - the base covers the whole footprint, the walls stand on it; holes 4.0
//   for the kit's "M3" (3.64 measured), pilots deeper than the screw reaches
// - the knob is held by its own M7 nut, not snap hooks (one didn't catch)
// - pre-print checks below fail the export
// Sizes in mm: part dimensions from their Homebox entries (MEASUREMENTS.md);
// values marked ASSUMED were not measured.
//
// Axes: X = width, left to right seen from the front; Y = depth, front at
// y = 0; Z = up, table at z = 0 (rubber feet not modelled).
//
// Printed parts:
//   body  - side walls, top with the knob's hole, back; open at the bottom
//           and the front
//   front - the tilted screen panel, a flat plate between the side walls on
//           two rails; the OLED is clamped onto it while it lies on the desk
//   base  - floor over the whole footprint, the vented strip under the
//           screen, sensor pockets, carrier standoffs, DS3231 holder;
//           4 screws up into the body's corner bosses
//   hood  - roof + back wall of the sensor bay, sits on the base
//   clamp - bar holding the OLED against the front plate, print 2
// Pick one with -D 'part="body"' (see export.sh). Single parts come out in
// print orientation, "assembly" shows everything in place with stand-in
// blocks for the modules, "clash" is empty when nothing overlaps.

part = "assembly"; // [assembly, body, front, base, hood, clamp, clash, dims]
cut = -1; // assembly only: >= 0 cuts the printed parts away left of this x
show_body = true; // assembly only: untick to see inside
show_front = true;
show_base = true;
show_hood = true;
show_clamps = true;
show_labels = true; // names over the module stand-ins
explode = 0; // assembly only: lifts the body, with the knob and the front plate, this far off the base
explode_front = 0; // ... and pulls the front plate, with the OLED on it, this far forward

$fn = 48;
eps = 0.01;

// ------------------------------------------------------------ print
clr = 0.3; // fit clearance between printed parts / around boards

// ------------------------------------------------------------ modules
oled_w = 26.00;
oled_h = 26.04; // pin edge at the top
oled_t = 2.64; // incl. glass
oled_pcb_t = 1.0; // ASSUMED, only used for the stand-in: rest of oled_t is glass
// lit area: caliper, all pixels on, front, pins up (2026-10-25; v1 used a
// photo guess and its window came out ~2 mm off toward the pin edge)
oled_lit_w = 21.69;
oled_lit_h = 11.03;
oled_lit_top = 5.27; // lit area's top edge below the pin edge
oled_lit_left = 1.86; // lit area's left edge from the board's left edge (seen from the front)
oled_back = 8.25; // header pins + plastic behind the PCB
// mounting holes, centred on the board; the back is full of SMD parts, pads
// and the flex: from each hole's edge to the nearest one (caliper,
// 2026-10-25) top-left 4.07, top-right 1.25, both bottom 0.77
oled_hole_x = 11.00; // centres 22.00 apart left-right
oled_hole_z = 10.76; // 21.52 apart top-bottom
oled_hole_d = 1.70;
oled_hole_room = 0.77; // the smallest of the four
oled_dupont = 14; // a plugged-in Dupont connector adds this

// ESP32 mini (D1 mini layout), metal can up
esp_l = 39.02; // antenna edge to the USB end
esp_w = 31.51; // across the pin rows
esp_t = 1.49;
esp_top = 4.66; // board bottom to the top of the metal can
esp_usb_z = 4.61 - 3.12 / 2; // USB-C centre above the board bottom
esp_usb_out = 1.22; // USB-C shell past the board edge
esp_antenna = 6; // antenna end, opposite the USB

ky_l = 26.18;
ky_w = 19.29;
ky_t = 1.47;
ky_cap_d = 14.78;
ky_shaft_y = 16 - ky_cap_d / 2; // shaft centre from the board's short edge away
                                // from the pins: ~8.6 (+-1)
ky_cap_h = 31.79 - 15.58; // cap height
ky_cap_above = 15.58 - ky_t; // board front face to the cap's lower edge, pushed on fully
ky_body = 12.5; // EC11 body footprint (ASSUMED, standard part)
ky_body_h = 6.38; // EC11 body height above the board
ky_collar_d = 6.73; // M7 threaded collar round the shaft
ky_collar_h = 6.86; // above the body
ky_shaft_d = 6; // (standard EC11)
ky_shaft_h = 11.83; // past the collar's end
ky_nut_af = 9.87; // M7 nut across the flats
ky_nut_t = 2.24;
ky_washer_t = 0.37;
ky_washer_d = 11; // ASSUMED (not needed)
ky_hdr_out = 6; // right-angle header: pins past the board's pin end (ASSUMED, ~6 from a photo)
ky_dupont = 14; // a plugged-in 5-pin Dupont housing + cable bend, past the pins' start

ds_l = 31.93;
ds_w = 21.79;
ds_h = 9.39;
ds_pins6 = 6 + 14 + 5; // 6-pin end: factory right-angle pins (standard ~6 mating length; the plug slides over them, so on the safe side) + a Dupont plug (14) + straight cable (5)
ds_pins4 = 8; // pass-through 4-pin end: factory right-angle pins

bme_l = 13.15;
bme_w = 10.66;
bme_h = 3;

scd_l = 21.81;
scd_w = 13.49;
scd_h = 7.59;

// carrier board: the Miuzei 4 x 6 cm perfboard (20 x 14 holes), uncut,
// on 4 standoffs. Holes drilled to match them: lay it on the printed base
// in its outline, mark through, drill 2.5 (2 x 6 screws pass)
perf_w = 60;
perf_d = 40;
perf_t = 1.6;
perf_standoff = 18; // the DS3231 lies on the floor under it, below the wires
perf_hole_inset = 3.5;
perf_pilot = 1.6; // 2 x 6 self-tapping, black kit (2.0 into 1.6 worked in mukklet)
hdr_h = 8.35 + 2.42; // carrier to the mini: female socket + the mini's own male header plastic
wire_h = 5; // soldered wires under the carrier, highest loop (~5, +-)
wire_clear = 2; // kept free below them
loop_out = 6; // white wire loops past the carrier's back edge, around cols 9-12
pin_plastic = 2.42; // module pin headers (standard, straight): plastic on the board
pin_tip = 8.28; // board to the pin tips
dupont = 14; // a Dupont jumper housing on the pins (user, 2026-10-20)
dupont_bend = 5; // straight cable above it before it can bend (ASSUMED)

// ------------------------------------------------------------ case
wall = 2.0;
tilt = 20; // screen panel, back from vertical
skirt_h = 25; // vented strip under the screen, in front of the sensor bay (22 until 2026-10-20: +3 so the front pin headers' plugs clear the knob)
panel_margin = 4; // screen panel beyond the OLED board, top and bottom
W = 80;
D = 84;
base_t = 3;

panel_len = oled_h + 2 * panel_margin; // along the slope
H = skirt_h + panel_len * cos(tilt);
run = panel_len * sin(tilt); // how far back the panel's top edge is

// base -> body: 4 x M3 x 10 round head, silver kit (3.64 x ~11.7 measured)
boss_d = 7;
boss_pilot = 2.5;
boss_hole = 4.0; // in the base: mukklet v1's 3.4 had to be drilled to 4
boss_cb_d = 6.5; // head counterbore under the base, 2 deep (1 mm of base left)
boss_cb_h = 2;
boss_pilot_h = 13; // from 1 mm under the base's top face
base_screw_l = 11.7; // the silver kit's "M3 x 10", measured
strip_drop = 0.8; // the base's strip stops this far below the skirt's edge
boss_in = wall + boss_d / 2 - 1; // centre from the outside; sunk 1 mm into the walls
boss_front = wall + strip_drop + boss_d / 2; // front pair: behind the base's strip, 0.8 clear
boss_xy = [[boss_in, boss_front], [W - boss_in, boss_front], [boss_in, D - boss_in], [W - boss_in, D - boss_in]];

// front plate: drops in between the side walls, flush with their edges,
// onto a rail along each side wall; 4 x 2 x 6 self-tapping pan head screws
// from the front (black kit), heads left proud. Mukklet v1 had 2.3 x 8
// into 1.9 pilots: too tight, two snapped
rail_w = 6; // from the side wall's inner face
rail_d = 8; // behind the plate
front_gap = 0.2; // plate's bottom edge above the skirt's edge (1.0 above the base's strip)
front_screw_d = 2.4; // clearance hole in the plate
front_pilot = 1.6;
front_pilot_depth = 6; // behind the plate: the screw reaches 4
front_screw_x = W / 2 - wall - rail_w / 2 - 0.5; // 2 mm of plate to its edge
front_screw_l = 6;
front_screw_v = [4.5, panel_len - 4.5]; // up the slope

// sensor bay: y wall..bay_back, closed by the hood's roof at hood_top
bay_back = 22;
hood_t = 1.6;
hood_top = skirt_h - 1;
bme_xy = [24, 12];
scd_xy = [52, 12];
ledge_h = 1.5; // sensors sit on corner ledges, air flows underneath
rim = 1.2;

// OLED: glass flat against the front plate's inner face, located by 4
// corner stops, pressed on by two printed clamp bars (part "clamp", print
// 2), each screwed to a post beside the board with a 2 x 4 (black kit).
// The posts stop clamp_press short of the PCB's back, so the bar presses
// on the board. Its corner holes aren't used: the glass reaches almost to
// them, no room for a screw post
oled_v = panel_len / 2; // board centre, up the slope from the skirt's edge
oled_post_x = oled_w / 2 + clr + 2.5; // clamp posts, left and right of the board
oled_post_d = 5.5;
oled_pilot = 1.6;
clamp_press = 0.3;
clamp_t = 2;
clamp_screw_l = 4;
clamp_overlap = 3; // how far a bar reaches over the board's back, along its side edge
post_h = oled_t - clamp_press; // above the plate's inner face
lit_dz = oled_h / 2 - oled_lit_top - oled_lit_h / 2; // lit centre above board centre
lit_dx = -oled_w / 2 + oled_lit_left + oled_lit_w / 2; // lit centre right of board centre (front view: +x = right)
win_margin = 1; // per side: covers the hand measurements (+-0.5) and print/fit play
win_w = oled_lit_w + 2 * win_margin;
win_h = oled_lit_h + 2 * win_margin;
win_edge = 1.2; // straight before the chamfer: a knife edge reads as a thin wall (hubs.com)

// knob on top, left: KY-040 board flat under the top, shaft end to the
// front, pins (right-angle header) to the back. Its M7 collar goes up
// through a 7.3 hole and the nut that came with it, on its washer, clamps
// the encoder body against the inside of the top. Two ribs beside the
// board's pin end stop it turning. Same mount as mukklet's second case
// (printed, works); snap hooks failed in its first
knob_x = 19; // board 1.4 clear of the left front rail
ky_y0 = 16; // board's front edge
knob_y = ky_y0 + ky_shaft_y;
knob_hole = 7.3; // 6.73 measured: FDM holes come out small
ky_face_z = H - wall - ky_body_h - 0.01; // board front (component) face; 0.01 so the clash check doesn't see the touching faces
ky_back_z = ky_face_z - ky_t;
knob_nut_z = H + ky_washer_t; // nut's underside
knob_cap_z = ky_face_z + ky_cap_above; // cap's lower edge, pushed on fully
rib_gap = 1; // board edge to rib: the shaft's position across the board is +-1
rib_t = 1.6;
rib_l = 8; // along the board, at its pin end
assert(ky_collar_h - wall - ky_washer_t - ky_nut_t >= 1.5, "not enough thread above the knob nut");
assert(knob_cap_z > knob_nut_z + ky_nut_t + 1, "knob cap sits on the nut");

// Everything on the carrier sits on its 2.54 hole grid: hole (col, row) is
// at [grid_x0 + col * p, grid_y0 + row * p]. 20 cols along X, 14 rows
// across Y, centred on the board (margins 5.87 along, 3.49 across; the
// user measured equal margins on all four sides, 2026-10-21). Row 0 at the front. The mini's four 10-pin sockets in rows 1,
// 2, 11, 12, cols 10-19 (it sits centred across the board); its USB end
// measured 2.5 past the carrier's right edge. Pin headers in cols 0-4
p = 2.54;
usb_gap = 1.5; // USB-C shell to the wall
esp_over = 2.5; // mini's board past the carrier's right edge
esp_x1 = W - wall - usb_gap - esp_usb_out;
esp_x0 = esp_x1 - esp_l;
perf_x1 = esp_x1 - esp_over;
perf_x0 = perf_x1 - perf_w;
grid_x0 = perf_x0 + 5.87;
grid_y0 = D - wall - boss_d - 1 - perf_d + 3.49;
perf_y0 = grid_y0 - 3.49;
perf_y1 = perf_y0 + perf_d;
perf_z = base_t + perf_standoff;
esp_y0 = grid_y0 + 6.5 * p - esp_w / 2;
esp_y1 = esp_y0 + esp_w;
esp_z = perf_z + perf_t + hdr_h; // mini board bottom
usb_y = (esp_y0 + esp_y1) / 2; // hole in the right wall (socket ~0.25 off centre: ignored)
usb_z = esp_z + esp_usb_z;
usb_hole = [13, 8];

// DS3231, on the floor in a holder, coin cell up, long axis left-to-right
// under the carrier, 6-pin end to the left. Box ds_b = [x0, y0, x1, y1]
ds_z = base_t + ledge_h;
ds_b = [29, (perf_y0 + perf_y1) / 2 - ds_w / 2, 29 + ds_l, (perf_y0 + perf_y1) / 2 + ds_w / 2];

echo(str("case ", W, " x ", D, " x ", H, " mm (W x D x H)"));
echo(str("usb centre z ", usb_z, ", knob board face z ", ky_face_z));
assert(perf_x1 < W - wall && perf_y1 < D - wall, "carrier hits a wall");
assert(esp_x1 + esp_usb_out < W - wall && esp_y1 < D - wall, "ESP32 hits a wall");
assert(perf_y1 + loop_out < D - wall, "wire loops hit the back wall");
assert(knob_x - ky_w / 2 > wall + rail_w + 1, "knob board too close to the left front rail");

// straight male pin headers on the carrier, pin line along X: [name, pins,
// col, row]. KNOB (GND + SW DT CLK) at the back; the four I2C ones (GND
// VCC SCL SDA) take any I2C module
hd = [["KNOB", 5, 0, 12], ["I2C", 4, 0, 9], ["I2C", 4, 0, 6], ["I2C", 4, 0, 3], ["I2C", 4, 0, 0]];
assert(perf_z - wire_h - wire_clear >= ds_z + ds_h, "no room for the DS3231 under the carrier's wires");

// ------------------------------------------------------------ pre-print checks
// What the clash check can't see (the enclosure-preprint-review skill;
// mukklet v1's misses: holes 0.2 from an edge, pilots too short for the
// screw, a 0.2 gap that closed). Fails the export.
function edge_ok(hole_d, centre_to_edge) = centre_to_edge - hole_d / 2 >= max(2, 0.6 * hole_d) - 0.01;
// plastic round the holes, to the nearest edge of the part they're in
assert(edge_ok(boss_hole, min(boss_in, boss_front)), "base screw hole too close to the base's edge");
assert(min(boss_in, boss_front) - boss_cb_d / 2 >= 1.2, "base screw counterbore too close to the base's edge");
plate_bottom_v = (front_gap + wall * sin(tilt)) / cos(tilt); // the plate's level-cut bottom edge, inner face, along the slope
assert(edge_ok(front_screw_d, W / 2 - wall - clr - front_screw_x), "front plate screw hole too close to the plate's side");
assert(edge_ok(front_screw_d, front_screw_v[0] - plate_bottom_v), "front plate screw hole too close to the plate's bottom");
assert(front_screw_x - front_pilot / 2 >= W / 2 - wall - rail_w + 1, "front pilot too close to the rail's inner edge");
// pilot depth against the screw's reach: >= 1 mm left at the tip
base_screw_tip = boss_cb_h + base_screw_l; // the head sits at the counterbore's top
assert(base_t - 1 + boss_pilot_h - base_screw_tip >= 1, "base screw bottoms out in its pilot");
assert(front_pilot_depth - (front_screw_l - wall) >= 1, "front plate screw bottoms out in its pilot");
clamp_reach = clamp_screw_l - clamp_t + clamp_press; // into the post
clamp_pilot_h = post_h + wall - 0.8; // 0.8 of plate left in front of it
assert(clamp_pilot_h - clamp_reach >= 1, "OLED clamp screw bottoms out in its pilot");
assert(perf_standoff - (6 - perf_t) >= 1, "carrier screw bottoms out in its standoff");
// gaps that aren't meant to touch: print tolerance + screw-hole play
assert(front_gap + strip_drop >= 0.8, "front plate too close to the base's strip");
assert(win_w > oled_lit_w && win_h > oled_lit_h, "window smaller than the lit area");
echo(str("CHECKS: base hole plastic ", min(boss_in, boss_front) - boss_hole / 2, ", counterbore ", min(boss_in, boss_front) - boss_cb_d / 2,
         "; plate hole to side ", W / 2 - wall - clr - front_screw_x - front_screw_d / 2, ", to bottom ", front_screw_v[0] - plate_bottom_v - front_screw_d / 2,
         "; pilot spare: base ", base_t - 1 + boss_pilot_h - base_screw_tip, ", front ", front_pilot_depth - (front_screw_l - wall),
         ", clamp ", clamp_pilot_h - clamp_reach, "; plate over strip ", front_gap + strip_drop,
         "; thread above the knob nut ", ky_collar_h - wall - ky_washer_t - ky_nut_t));

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

function hole(col, row) = [grid_x0 + col * p, grid_y0 + row * p];

// ------------------------------------------------------------ front plate
// two posts beside the board for the clamp bars, and L-shaped corner stops;
// the flex cable passes between the bottom two
module oled_mount() oled_frame() {
  for (sx = [-1, 1]) translate([sx * oled_post_x, -0.5, 0]) rotate([-90, 0, 0]) cylinder(d = oled_post_d, h = post_h + 0.5);
  for (sx = [-1, 1], sz = [-1, 1]) {
    ex = oled_w / 2 + clr;
    ez = oled_h / 2 + clr;
    box([min(sx * ex, sx * (ex + 1.2)), -0.5, min(sz * (ez - 4), sz * (ez + 1.2))], [max(sx * ex, sx * (ex + 1.2)), 2, max(sz * (ez - 4), sz * (ez + 1.2))]);
    box([min(sx * (ex - 4), sx * (ex + 1.2)), -0.5, min(sz * ez, sz * (ez + 1.2))], [max(sx * (ex - 4), sx * (ex + 1.2)), 2, max(sz * ez, sz * (ez + 1.2))]);
  }
}

module oled_pilots() oled_frame() for (sx = [-1, 1])
  translate([sx * oled_post_x, post_h + eps, 0]) rotate([90, 0, 0]) cylinder(d = oled_pilot, h = clamp_pilot_h);

// clamp bar, in oled_frame coordinates for the right side (mirrored for the left)
module clamp_bar() difference() {
  box([oled_w / 2 - clamp_overlap, oled_t, -oled_h / 2 + 1], [oled_post_x + oled_post_d / 2, oled_t + clamp_t, oled_h / 2 - 1]);
  translate([oled_post_x, oled_t - 1, 0]) rotate([-90, 0, 0]) cylinder(d = 2.4, h = clamp_t + 2);
}

// a straight cut through, and a 45 degree chamfer outwards that starts
// win_edge in front of the inner face (no eps-thin slabs: hubs.com's
// checker saw a skin across mukklet's window built from those)
module oled_window() oled_frame() translate([lit_dx, 0, lit_dz]) {
  c = wall - win_edge + 1; // the chamfer's spread 1 mm in front of the face
  translate([0, -wall / 2, 0]) cube([win_w, wall + 2, win_h], center = true);
  hull() {
    translate([0, -win_edge + 0.25, 0]) cube([win_w, 0.5, win_h], center = true);
    translate([0, -wall - 1.25, 0]) cube([win_w + 2 * c, 0.5, win_h + 2 * c], center = true);
  }
}

module front_holes(d, depth, from = -1) panel_frame() for (sx = [-1, 1], v = front_screw_v)
  translate([sx * front_screw_x, from, v]) rotate([-90, 0, 0]) cylinder(d = d, h = depth);

module front() difference() {
  union() {
    intersection() {
      outer();
      panel_frame() box([-W / 2 + wall + clr, 0, -5], [W / 2 - wall - clr, wall, panel_len + 5]);
      box([0, -1, skirt_h + front_gap], [W, D, H]);
    }
    oled_mount();
  }
  oled_window();
  oled_pilots();
  front_holes(front_screw_d, wall + 2);
}

// ------------------------------------------------------------ body
// what the front plate and the base's strip take: everything between the
// side walls in front of the plate's inner face, and of the strip below it
// (eps deeper than the plate, so the clash check sees a gap, not a touch)
module front_cut() {
  w = wall + eps;
  extrude_x(wall, W - 2 * wall) polygon([
    [-5, -5], [wall + clr, -5], [wall + clr, skirt_h + (wall + clr - w * cos(tilt)) / tan(tilt) - w * sin(tilt)],
    [w * cos(tilt) + 80 * sin(tilt), skirt_h - w * sin(tilt) + 80 * cos(tilt)], [-5, H + 20]]);
}

// a rail along each side wall, the plate lies on them (the cut trims their
// front to the plate's inner face); from above the hood's roof to the top,
// so nothing overhangs with the body printed upside down
module front_rails() intersection() {
  panel_frame() for (sx = [-1, 1])
    box([min(sx * (W / 2 - wall + 0.5), sx * (W / 2 - wall - rail_w)), wall - 0.5, -30],
        [max(sx * (W / 2 - wall + 0.5), sx * (W / 2 - wall - rail_w)), wall + rail_d, panel_len + 10]);
  box([0, 0, hood_top + 1], [W, D, H]);
}

module bosses() for (p = boss_xy) translate([p.x, p.y, base_t + eps]) cylinder(d = boss_d, h = H);

module boss_pilots() for (p = boss_xy) translate([p.x, p.y, base_t - 1]) cylinder(d = boss_pilot, h = boss_pilot_h);

// two ribs beside the board's long edges at its pin end, down past the
// board: the nut holds the encoder, these only stop it turning
module knob_mount() for (sx = [-1, 1]) {
  e = knob_x + sx * (ky_w / 2 + rib_gap);
  box([min(e, e + sx * rib_t), ky_y0 + ky_l - rib_l, ky_back_z - 1.5], [max(e, e + sx * rib_t), ky_y0 + ky_l, H - wall + 0.5]);
}

module vents() {
  // bay sides, behind the front bosses
  for (x = [-1, W - wall - 1]) for (z = [6:4:skirt_h - 5]) box([x, 11, z], [x + wall + 2, bay_back - 2, z + 2]);
  // top, over the ESP32: its warm air leaves here
  slots(esp_x0 + 2, esp_x1 - 2, esp_y0 + 2, esp_y1 - 2, H - wall - 1, H + 1);
  // back, low: intake
  slots(14, W - 14, D - wall - 1, D + 1, base_t + 2, base_t + 9);
}

module body() difference() {
  union() {
    difference() {
      outer();
      cavity();
    }
    intersection() {
      outer();
      union() {
        bosses();
        front_rails();
        knob_mount();
      }
    }
  }
  front_cut();
  front_holes(front_pilot, front_pilot_depth, wall - eps);
  boss_pilots();
  box([-1, -1, -1], [W + 1, D + 1, base_t]); // the walls stand on the base
  vents();
  translate([W - wall - 1, usb_y, usb_z]) rotate([0, 0, -90]) stadium(usb_hole[0], usb_hole[1], wall + 2);
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
module ds_place() translate([ds_b[0], ds_b[3], 0]) rotate([0, 0, -90]) children();

// rails along the long edges and 4 corner ledges; the ends stay open for the pins
module ds_holder() ds_place() translate([0, 0, base_t - eps]) {
  h = ledge_h + 1.6 + 1;
  for (x = [-clr - rim, ds_w + clr]) box([x, 2, 0], [x + rim, ds_l - 2, h]);
  for (x = [0, ds_w - 2.5], y = [0, ds_l - 2.5]) box([x, y, 0], [x + 2.5, y + 2.5, ledge_h]);
}

perf_holes = [for (x = [perf_x0 + perf_hole_inset, perf_x1 - perf_hole_inset])
    for (y = [perf_y0 + perf_hole_inset, perf_y1 - perf_hole_inset]) [x, y]];

// the vented strip under the screen, between the side walls
module strip() difference() {
  box([wall + clr, 0, base_t - eps], [W - wall - clr, wall, skirt_h - strip_drop]);
  slots(14, W - 14, -1, wall + 1, 6, skirt_h - 5);
}

module base() {
  x0 = wall + clr;
  difference() {
    box([0, 0, 0], [W, D, base_t]);
    for (p = boss_xy) translate([p.x, p.y, -1]) {
      cylinder(d = boss_hole, h = base_t + 2);
      cylinder(d = boss_cb_d, h = 1 + boss_cb_h);
    }
    slots(12, W - 12, 5, bay_back - 3, -1, base_t + 1); // under the sensors
    slots(12, W - 12, bay_back + hood_t + 3, min(perf_y0, ds_b[1] - 2) - 1, -1, base_t + 1); // intake in front of the carrier
  }
  strip();
  pocket(bme_xy, bme_l, bme_w);
  pocket(scd_xy, scd_l, scd_w);
  ds_holder();
  for (p = perf_holes) translate([p.x, p.y, base_t - eps]) difference() {
    cylinder(d = 6, h = perf_standoff);
    cylinder(d = perf_pilot, h = perf_standoff + 1);
  }
  // two ribs the hood's back wall drops between, gap at the cable notch
  for (y = [bay_back - clr - 1.2, bay_back + hood_t + clr]) for (xs = [[x0 + 10, W / 2 - 9], [W / 2 + 9, W - x0 - 10]])
    box([xs[0], y, base_t - eps], [xs[1], y + 1.2, base_t + 2]);
}

// ------------------------------------------------------------ hood
module hood() difference() {
  x0 = wall + clr;
  union() {
    box([x0, bay_back, base_t + eps], [W - x0, bay_back + hood_t, hood_top]);
    box([x0, wall + clr, hood_top - hood_t], [W - x0, bay_back + hood_t, hood_top]);
  }
  for (p = boss_xy) translate([p.x - boss_d / 2 - 0.5, p.y - boss_d / 2 - 0.5, 0]) cube([boss_d + 1, boss_d + 1, H]);
  box([W / 2 - 8, bay_back - 1, base_t - 1], [W / 2 + 8, bay_back + hood_t + 1, base_t + 6]); // sensor cables
}

// ------------------------------------------------------------ stand-ins
// one pin header, centred on its pin line, z 0 = the carrier's top
module hd_frame(c) let(at = hole(c[2], c[3])) translate([at.x + (c[1] - 1) * p / 2, at.y, perf_z + perf_t]) children();

// plastic, and a plugged-in Dupont housing + straight cable as a see-through block
module hd_header(c) hd_frame(c) {
  l = c[1] * p;
  color("dimgray") box([-l / 2, -p / 2, eps], [l / 2, p / 2, pin_plastic]);
  color("gold", 0.35) box([-l / 2 - 0.2, -p / 2 - 0.2, pin_plastic + eps], [l / 2 + 0.2, p / 2 + 0.2, pin_plastic + dupont + dupont_bend]);
}

module hd_label(c) hd_frame(c) color("black") translate([0, 0, pin_plastic + dupont + dupont_bend + 0.3])
  linear_extrude(0.3) text(c[0], size = 2, halign = "center", valign = "center");

// the soldered wires under the carrier (standoffs left free), and the white
// loops past its back edge
module wires_standin() color("white", 0.6) {
  difference() {
    box([perf_x0, perf_y0, perf_z - wire_h], [perf_x1, perf_y1, perf_z - eps]);
    for (q = perf_holes) translate([q.x, q.y, 0]) cylinder(d = 6 + 2 * clr, h = H);
  }
  box([hole(9, 0).x - p, perf_y1 + eps, perf_z - wire_h], [hole(12, 0).x + p, perf_y1 + loop_out, perf_z + perf_t]);
}

module oled_standin() color("steelblue") oled_frame() {
  box([-oled_w / 2, oled_t - oled_pcb_t, -oled_h / 2], [oled_w / 2, oled_t - eps, oled_h / 2]);
  translate([0, 0, lit_dz]) box([-oled_w / 2, eps, -8], [oled_w / 2, oled_t - oled_pcb_t, 8]);
  box([-5.1, oled_t + eps, oled_h / 2 - 2.54], [5.1, oled_t + oled_back, oled_h / 2]);
  color("gold", 0.35) box([-5.1, oled_t + oled_back, oled_h / 2 - 2.54], [5.1, oled_t + oled_back + oled_dupont, oled_h / 2]);
}

// the mini: its sockets (two 10-pin pairs, rows 1-2 and 11-12) up to its
// board, can and USB-C on top, antenna end left
module esp_standin() {
  color("black") for (r = [1, 11])
    box([hole(10, r).x - p / 2, hole(10, r).y - p / 2, perf_z + perf_t], [hole(19, r).x + p / 2, hole(10, r + 1).y + p / 2, esp_z]);
  color("royalblue") box([esp_x0, esp_y0, esp_z], [esp_x1, esp_y1, esp_z + esp_t]);
  color("silver") box([esp_x0 + esp_antenna + 1, esp_y0 + 6, esp_z + esp_t], [esp_x0 + 26, esp_y1 - 6, esp_z + esp_top]);
  color("silver") box([esp_x1 - 7, usb_y - 4.45, esp_z + esp_t], [esp_x1 + esp_usb_out, usb_y + 4.45, esp_z + esp_top]);
  color("orange", 0.5) box([esp_x0, esp_y0, esp_z + esp_t], [esp_x0 + esp_antenna, esp_y1, esp_z + esp_top]); // antenna: keep clear
}

module ds_standin() ds_place() {
  color("navy") box([0, 0, ds_z], [ds_w, ds_l, ds_z + ds_h]);
  color("dimgray", 0.6) {
    box([ds_w / 2 - 7.6, -ds_pins6, ds_z + 1.6], [ds_w / 2 + 7.6, -eps, ds_z + 4.1]); // 6-pin + cable
    box([ds_w / 2 - 5.1, ds_l + eps, ds_z + 1.6], [ds_w / 2 + 5.1, ds_l + ds_pins4, ds_z + 4.1]); // 4-pin
  }
}

module knob_standin() {
  color("firebrick") {
    box([knob_x - ky_w / 2, ky_y0, ky_back_z], [knob_x + ky_w / 2, ky_y0 + ky_l, ky_face_z]);
    translate([knob_x, knob_y, ky_face_z + ky_body_h / 2]) cube([ky_body, ky_body, ky_body_h], center = true);
  }
  bt = ky_face_z + ky_body_h;
  color("silver") translate([knob_x, knob_y, bt]) {
    cylinder(d = ky_collar_d, h = ky_collar_h);
    cylinder(d = ky_shaft_d, h = ky_collar_h + ky_shaft_h);
  }
  color("dimgray") translate([knob_x, knob_y, H + 0.01]) {
    cylinder(d = ky_washer_d, h = ky_washer_t);
    translate([0, 0, ky_washer_t]) cylinder(d = ky_nut_af / cos(30), h = ky_nut_t, $fn = 6);
  }
  // right-angle header on the component face at the pin end, pins
  // pointing back, a Dupont plug on them
  y1 = ky_y0 + ky_l;
  color("dimgray") box([knob_x - 2.5 * p, y1 - p, ky_face_z], [knob_x + 2.5 * p, y1, ky_face_z + p]);
  color("gold", 0.35) box([knob_x - 2.5 * p - 0.6, y1 + eps, ky_face_z - 0.3], [knob_x + 2.5 * p + 0.6, y1 + ky_hdr_out + ky_dupont, ky_face_z + p + 0.3]);
  color("gray") translate([knob_x, knob_y, knob_cap_z]) cylinder(d = ky_cap_d, h = ky_cap_h);
}

// everything on the base: carrier, ESP32, sockets, DS3231, sensors
module base_standins() {
  color("darkgreen") box([perf_x0, perf_y0, perf_z], [perf_x1, perf_y1, perf_z + perf_t]);
  esp_standin();
  for (c = hd) hd_header(c);
  wires_standin();
  ds_standin();
  color("purple") {
    box([bme_xy.x - bme_l / 2, bme_xy.y - bme_w / 2, base_t + ledge_h], [bme_xy.x + bme_l / 2, bme_xy.y + bme_w / 2, base_t + ledge_h + bme_h]);
    box([scd_xy.x - scd_l / 2, scd_xy.y - scd_w / 2, base_t + ledge_h], [scd_xy.x + scd_l / 2, scd_xy.y + scd_w / 2, base_t + ledge_h + scd_h]);
  }
}

module stand_ins() {
  oled_standin();
  knob_standin();
  base_standins();
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
  for (c = hd) hd_label(c);
  label("OLED", panel_pt(0, wall + oled_t + oled_back, oled_v + oled_h / 2), 13);
  label("ESP32 mini", [(esp_x0 + esp_x1) / 2, usb_y, esp_z + esp_top], 3);
  label("antenna", [esp_x0 + 3, usb_y, esp_z + esp_top], 13);
  label("USB-C", [esp_x1, usb_y, esp_z + esp_top], 18);
  label("carrier", [perf_x0 + 6, perf_y0 + 3, perf_z + perf_t], 8);
  label("DS3231", [(ds_b[0] + ds_b[2]) / 2, (ds_b[1] + ds_b[3]) / 2, ds_z + ds_h], 25);
  label("KY-040", [knob_x, knob_y, knob_cap_z + ky_cap_h], 20);
  label("BME280", [bme_xy.x, bme_xy.y, base_t + ledge_h + bme_h], 8);
  label("SCD41", [scd_xy.x, scd_xy.y, base_t + ledge_h + scd_h], 8);
}

// ------------------------------------------------------------ output
// assembly only: the printed parts cut away left of x = cut
module cutaway() intersection() {
  children();
  if (cut >= 0) box([cut, -10, -10], [W + 10, D + 10, H + 10]);
  else box([-10, -10, -10], [W + 10, D + 10, H + 10]);
}

module clamps() oled_frame() for (m = [0, 1]) mirror([m, 0, 0]) clamp_bar();

if (part == "body") translate([0, D, H]) rotate([180, 0, 0]) body();
else if (part == "front") rotate([90, 0, 0]) rotate([tilt, 0, 0]) translate([-W / 2, 0, -skirt_h]) front(); // face down
else if (part == "base") base();
else if (part == "clamp") for (i = [0, 1]) translate([i * 12, 0, 0])
  translate([0, 0, -oled_t]) rotate([90, 0, 0]) clamp_bar();
else if (part == "hood") translate([0, 0, hood_top]) rotate([180, 0, 0]) hood();
else if (part == "dims") echo(W = W, D = D, H = H, wall = wall, tilt = tilt, skirt_h = skirt_h, run = run,
  panel_len = panel_len, oled_w = oled_w, oled_h = oled_h, oled_v = oled_v, lit_dz = lit_dz, win_w = win_w,
  win_h = win_h, knob_x = knob_x, knob_y = knob_y, knob_hole = knob_hole, ky_y0 = ky_y0, ky_l = ky_l, ky_w = ky_w,
  usb_x = W, usb_y = usb_y, usb_z = usb_z, usb_w = usb_hole[0], usb_h = usb_hole[1], usb_wall = 1, boss_in = boss_in, boss_d = boss_d,
  bay_back = bay_back, hood_t = hood_t, bme_x = bme_xy.x, bme_y = bme_xy.y, bme_l = bme_l, bme_w = bme_w,
  scd_x = scd_xy.x, scd_y = scd_xy.y, scd_l = scd_l, scd_w = scd_w, perf_x0 = perf_x0, perf_y0 = perf_y0,
  perf_w = perf_w, perf_d = perf_d, esp_x0 = esp_x0, esp_y0 = esp_y0, esp_w = esp_x1 - esp_x0, esp_l = esp_y1 - esp_y0,
  esp_antenna = esp_antenna, ant_x = esp_x0 + 1, ant_y = esp_y0 - 1.5,
  ds_x0 = ds_b[0], ds_y0 = ds_b[1], ds_w = ds_b[2] - ds_b[0], ds_l = ds_b[3] - ds_b[1], ds_under = 1); // for cardboard.py
else if (part == "clash") {
  // printed parts against every stand-in
  intersection() {
    union() {
      body();
      front();
      base();
      hood();
      clamps();
    }
    stand_ins();
  }
  // printed parts against each other
  intersection() {
    body();
    union() {
      front();
      base();
      hood();
      clamps();
    }
  }
  intersection() {
    front();
    union() {
      base();
      hood();
    }
  }
  intersection() {
    hood();
    base();
  }
  // stand-ins against each other (plugs included)
  intersection() {
    oled_standin();
    union() {
      knob_standin();
      base_standins();
    }
  }
  intersection() {
    knob_standin();
    base_standins();
  }
  // headers and plugs against the ESP32 (and its sockets); wires against the DS3231
  intersection() {
    for (c = hd) hd_header(c);
    esp_standin();
  }
  intersection() {
    wires_standin();
    ds_standin();
  }
}
else {
  if (show_base) cutaway() color("khaki") base();
  if (show_hood) cutaway() color("tan") hood();
  base_standins();
  translate([0, 0, explode]) {
    if (show_body) cutaway() color("white") body();
    knob_standin();
    translate([0, -explode_front, 0]) {
      if (show_front) cutaway() color("gainsboro") front();
      if (show_clamps) color("dimgray") clamps();
      oled_standin();
    }
    if (show_labels) labels();
  }
}
