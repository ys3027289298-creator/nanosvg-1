#!/usr/bin/env python3
"""Rendered RGBA checks for feFlood / feOffset (no third-party deps)."""
import os
import struct
import sys

W = H = 64

CASES = [
    ("flood_solid",       (255, 0, 0),   (0, 0, 63, 63)),
    ("offset10",          (0, 170, 0),   (22, 22, 41, 41)),
    ("flood_offset_chain", (0, 0, 255),  (10, 10, 63, 63)),
    ("regression",        (51, 102, 204), None),
]

def load_rgba(path):
    data = open(path, "rb").read()
    return [struct.unpack_from("BBBB", data, i * 4) for i in range(len(data) // 4)]

def bbox_and_color(px):
    xs, ys, colors = [], [], {}
    for i, (r, g, b, a) in enumerate(px):
        if a == 0:
            continue
        x, y = i % W, i // W
        xs.append(x); ys.append(y)
        if a == 255:
            colors[(r, g, b)] = colors.get((r, g, b), 0) + 1
    box = (min(xs), min(ys), max(xs), max(ys))
    main_color = max(colors, key=colors.get)
    return box, main_color

def main(out_dir):
    ok = True
    for name, expect_color, expect_box in CASES:
        px = load_rgba(os.path.join(out_dir, name + ".rgba"))
        box, color = bbox_and_color(px)
        print("%-20s bbox=%s color=#%02x%02x%02x" % (name, box, *color))
        if expect_box is not None and box != expect_box:
            ok = False
            print("  FAIL bbox: want %s got %s" % (expect_box, box))
        if expect_color != color:
            ok = False
            print("  FAIL color: want #%02x%02x%02x" % expect_color)
    print("RESULT:", "PASS" if ok else "FAIL")
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else "output"))
