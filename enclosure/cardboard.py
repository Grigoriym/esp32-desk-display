#!/usr/bin/env python3
"""Cardboard mock-up templates of the enclosure, 1:1, as a 2-page A4 PDF.

Sizes come from enclosure.scad (part="dims"), so the templates follow the
model. Print at 100% / "Actual size" and check the 50 mm bar with a ruler.

Usage: enclosure/cardboard.py [cardboard thickness in mm, default 2]
Writes enclosure/cardboard.pdf (gitignored).
"""
import math
import re
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
CT = float(sys.argv[1]) if len(sys.argv) > 1 else 2.0  # cardboard thickness
PT = 72 / 25.4  # PDF points per mm
PAGE_W, PAGE_H = 210, 297


def dims():
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "d.echo"
        subprocess.run(["openscad", "-D", 'part="dims"', "-o", str(out), "--export-format=echo",
                        str(HERE / "enclosure.scad")], check=True, capture_output=True)
        line = next(l for l in out.read_text().splitlines() if l.startswith("ECHO: W ="))
    return {k: float(v) for k, v in re.findall(r"(\w+) = ([-\d.e]+)", line)}


class Page:
    """Drawing in mm, origin top-left, y down."""

    def __init__(self):
        self.ops = []

    def _p(self, x, y):
        return f"{x * PT:.2f} {(PAGE_H - y) * PT:.2f}"

    def style(self, width=0.3, dash=False, grey=0.0):
        self.ops.append(f"{width * PT:.2f} w {'[3 2] 0 d' if dash else '[] 0 d'} {grey} G")

    def poly(self, pts, close=True, **st):
        self.style(**st)
        self.ops.append(" ".join([f"{self._p(*pts[0])} m"] + [f"{self._p(*p)} l" for p in pts[1:]])
                        + (" h S" if close else " S"))

    def rect(self, x, y, w, h, **st):
        self.poly([(x, y), (x + w, y), (x + w, y + h), (x, y + h)], **st)

    def circle(self, cx, cy, r, **st):
        k = 0.5523 * r
        self.style(**st)
        pts = [(cx + r, cy), (cx, cy - r), (cx - r, cy), (cx, cy + r)]
        ctl = [((cx + r, cy - k), (cx + k, cy - r)), ((cx - k, cy - r), (cx - r, cy - k)),
               ((cx - r, cy + k), (cx - k, cy + r)), ((cx + k, cy + r), (cx + r, cy + k))]
        s = f"{self._p(*pts[0])} m"
        for i in range(4):
            s += f" {self._p(*ctl[i][0])} {self._p(*ctl[i][1])} {self._p(*pts[(i + 1) % 4])} c"
        self.ops.append(s + " S")

    def stadium(self, cx, cy, w, h, **st):  # rounded ends left/right
        r = h / 2
        self.poly([(cx - w / 2 + r, cy - r), (cx + w / 2 - r, cy - r)], close=False, **st)
        self.poly([(cx - w / 2 + r, cy + r), (cx + w / 2 - r, cy + r)], close=False, **st)
        for sx in (-1, 1):  # half circles as short polylines
            c = cx + sx * (w / 2 - r)
            self.poly([(c + sx * r * math.sin(a), cy - r * math.cos(a))
                       for a in [i * math.pi / 16 for i in range(17)]], close=False, **st)

    def text(self, x, y, s, size=3.0, center=False):
        s = s.replace("\\", "\\\\").replace("(", "\\(").replace(")", "\\)")
        if center:
            x -= len(s) * size * 0.25  # rough Helvetica width
        self.ops.append(f"BT /F1 {size * PT:.1f} Tf {self._p(x, y)} Td ({s}) Tj ET")

    def scale_bar(self, x, y):
        self.poly([(x, y), (x + 50, y)], close=False, width=0.5)
        for i in range(0, 51, 10):
            self.poly([(x + i, y - (2 if i % 50 else 3)), (x + i, y)], close=False, width=0.3)
        self.text(x + 53, y + 1, "50 mm: check with a ruler, else print at 100% / Actual size", 3)


def write_pdf(pages, path):
    objs = ["<< /Type /Catalog /Pages 2 0 R >>", None,
            "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>"]
    kids = []
    for pg in pages:
        stream = "\n".join(pg.ops).encode("latin-1")
        objs.append(f"<< /Length {len(stream)} >>\nstream\n".encode("latin-1") + stream + b"\nendstream")
        objs.append(f"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 {PAGE_W * PT:.2f} {PAGE_H * PT:.2f}] "
                    f"/Resources << /Font << /F1 3 0 R >> >> /Contents {len(objs)} 0 R >>")
        kids.append(f"{len(objs)} 0 R")
    objs[1] = f"<< /Type /Pages /Kids [{' '.join(kids)}] /Count {len(pages)} >>"
    out = bytearray(b"%PDF-1.4\n")
    offsets = []
    for i, o in enumerate(objs, 1):
        offsets.append(len(out))
        out += f"{i} 0 obj\n".encode() + (o if isinstance(o, bytes) else o.encode("latin-1")) + b"\nendobj\n"
    xref = len(out)
    out += f"xref\n0 {len(objs) + 1}\n0000000000 65535 f \n".encode()
    out += "".join(f"{o:010d} 00000 n \n" for o in offsets).encode()
    out += f"trailer\n<< /Size {len(objs) + 1} /Root 1 0 R >>\nstartxref\n{xref}\n%%EOF\n".encode()
    path.write_bytes(out)


def main():
    d = dims()
    W, D, H = d["W"], d["D"], d["H"]
    iw = W - 2 * CT  # pieces that sit between the two sides
    M = 12  # page margin
    solid = dict(width=0.35)
    dash = dict(width=0.25, dash=True, grey=0.4)

    # ---- page 1: sides, front strip, screen panel
    p1 = Page()
    p1.text(M, 10, f"Desk display case: cardboard mock-up, page 1/2  (case {W:.0f} x {D:.0f} x {H:.0f} mm, "
                   f"cardboard {CT:g} mm)", 3.5)
    p1.scale_bar(M, 17)
    y = 30
    for n in (1, 2):
        # side profile, front on the left
        pts = [(0, 0), (0, d["skirt_h"]), (d["run"], H), (D, H), (D, 0)]
        p1.poly([(M + a, y + H - b) for a, b in pts], **solid)
        # sensor bay behind the front strip, for orientation
        p1.rect(M + CT, y + H - d["skirt_h"] + 1, d["bay_back"] - CT, d["skirt_h"] - 1 - CT, **dash)
        p1.text(M + D + 4, y + 8, f"SIDE {n} of 2", 4)
        p1.text(M + D + 4, y + 14, "front edge on the left", 3)
        p1.text(M + D + 4, y + 19, "dashed: sensor bay inside", 3)
        p1.text(M + D + 4, y + 24, "cut out along the solid line", 3)
        y += H + 10
    p1.rect(M, y, iw, d["skirt_h"], **solid)
    p1.text(M + iw + 4, y + 8, "FRONT STRIP (under the screen)", 4)
    p1.text(M + iw + 4, y + 14, "vents go here; glue between the sides", 3)
    y += d["skirt_h"] + 10
    pl = d["panel_len"]
    p1.rect(M, y, iw, pl, **solid)
    cx = M + iw / 2
    wy = y + pl - (d["oled_v"] + d["lit_dz"])  # window centre, from the bottom edge
    p1.rect(cx - d["win_w"] / 2, wy - d["win_h"] / 2, d["win_w"], d["win_h"], **solid)
    oy = y + pl - d["oled_v"]
    p1.rect(cx - d["oled_w"] / 2, oy - d["oled_h"] / 2, d["oled_w"], d["oled_h"], **dash)
    p1.text(M + iw + 4, y + 8, "SCREEN PANEL", 4)
    p1.text(M + iw + 4, y + 14, "cut out the window; bottom edge", 3)
    p1.text(M + iw + 4, y + 19, "meets the front strip, tilted 20 deg", 3)
    p1.text(M + iw + 4, y + 24, "dashed: OLED board behind it", 3)
    p1.text(M + iw + 4, y + 29, "(pins at the top)", 3)

    # ---- page 2: top, back, floor layout
    p2 = Page()
    p2.text(M, 10, "Desk display case: cardboard mock-up, page 2/2", 3.5)
    p2.scale_bar(M, 17)
    y = 28
    tl = D - d["run"]  # top, from the panel's top edge to the back
    p2.rect(M, y, iw, tl, **solid)
    p2.circle(M + d["knob_x"] - CT, y + tl - (d["knob_y"] - d["run"]), d["knob_hole"] / 2, **solid)
    vent_y1 = D - d["wall"] - 4  # vent slots' world y range, as in the model
    vent_y0 = d["esp_y0"] + 14
    p2.rect(M + d["esp_x0"] + 2 - CT, y + D - vent_y1, d["esp_w"] - 4, vent_y1 - vent_y0, **dash)
    p2.text(M + iw + 4, y + 8, "TOP", 4)
    p2.text(M + iw + 4, y + 14, "front edge (to the screen panel)", 3)
    p2.text(M + iw + 4, y + 19, "at the BOTTOM; cut the knob hole", 3)
    p2.text(M + iw + 4, y + 24, "dashed: vent slots over the ESP32", 3)
    y += tl + 8
    p2.rect(M, y, iw, H, **solid)
    p2.stadium(M + (W - CT) - d["usb_x"], y + H - d["usb_z"], d["usb_w"], d["usb_h"], **solid)
    p2.text(M + iw + 4, y + 8, "BACK (seen from behind)", 4)
    p2.text(M + iw + 4, y + 14, "cut out the USB-C hole", 3)
    y += H + 8
    # floor: the base, with the modules' outlines to lay the real ones on; top view, front at the bottom
    fy = lambda wy: y + D - wy  # noqa: E731
    p2.rect(M, y, W, D, **solid)
    p2.rect(M + d["wall"], y + d["wall"], W - 2 * d["wall"], D - 2 * d["wall"], **dash)
    for bx in (d["boss_in"], W - d["boss_in"]):
        for by in (d["boss_in"], D - d["boss_in"]):
            p2.circle(M + bx, fy(by), d["boss_d"] / 2, **dash)
    p2.rect(M + d["wall"], fy(d["bay_back"] + d["hood_t"]), W - 2 * d["wall"], d["hood_t"], **dash)
    p2.text(M + 10, fy(3), "sensor bay", 2.5)

    def part(x0, y0, w, l, name, style=solid):
        p2.rect(M + x0, fy(y0 + l), w, l, **style)
        p2.text(M + x0 + 1, fy(y0 + l) + 3.5, name, 2.5)

    part(d["bme_x"] - d["bme_l"] / 2, d["bme_y"] - d["bme_w"] / 2, d["bme_l"], d["bme_w"], "BME280")
    part(d["scd_x"] - d["scd_l"] / 2, d["scd_y"] - d["scd_w"] / 2, d["scd_l"], d["scd_w"], "SCD41")
    part(d["perf_x0"], d["perf_y0"], d["perf_w"], d["perf_d"], "carrier", dash)
    part(d["ds_x0"], d["ds_y0"], d["ds_w"], d["ds_l"], "DS3231")
    part(d["esp_x0"], d["esp_y0"], d["esp_w"], d["esp_l"], "ESP32 (USB at the back)")
    p2.text(M + d["esp_x0"] + 1, fy(d["esp_y0"]) - 1.5, "antenna", 2.5)
    p2.text(M + 2, fy(-4) + 0.5, "FRONT", 3)
    p2.text(M + W + 4, y + 8, "FLOOR", 4)
    for i, s in enumerate(["cut the outer border only: the",
                           "boxes inside are outlines, not holes",
                           "top view, front at the bottom",
                           "lay the real modules on it",
                           "solid: parts on the floor / carrier",
                           "dashed: inside walls, screw posts,",
                           "sensor bay wall, carrier board",
                           f"KY-040 hangs under the top at x {d['knob_x']:.0f}",
                           "(above the DS3231), not drawn"]):
        p2.text(M + W + 4, y + 14 + i * 5, s, 3)

    out = HERE / "cardboard.pdf"
    write_pdf([p1, p2], out)
    print(out)


main()
