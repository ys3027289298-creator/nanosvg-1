#!/usr/bin/env python3
"""Measure rendered stroke width (px) and dash pitch for the veffect cases."""
import os
import sys

import numpy as np
from PIL import Image

# name -> (expected stroke width px, expected dash/gap pitch px)
CASES = [
    ("01_1x_nonscaling",            6.0, (14, 10)),
    ("02_2x_viewbox_nonscaling",    6.0, (14, 10)),
    ("03_2x_transform_nonscaling",  6.0, (14, 10)),
    ("04_2x_regression_default",   12.0, (28, 20)),
]

def runs_of(col):
    runs, cur, n = [], col[0], 1
    for v in col[1:]:
        if v == cur:
            n += 1
        else:
            runs.append((bool(cur), n))
            cur, n = v, 1
    runs.append((bool(cur), n))
    return runs

def measure(png):
    dark = np.asarray(Image.open(png).convert("L")) < 128
    h = dark.shape[0]
    # Locate the vertical segment: columns with a tall dark extent.
    vert = np.where(dark.sum(axis=0) > h * 0.25)[0]
    x = int((vert.min() + vert.max()) / 2)
    # Width from middle-third rows, away from the start cap and the corner.
    y0, y1 = int(h * 0.35), int(h * 0.62)
    row_widths = [int(row.sum()) for row in dark[y0:y1, :] if row.sum()]
    width = float(np.median(row_widths))
    # Dash pitch: mode of dark/light runs; edge runs are clipped, the mode
    # always corresponds to the full dash and gap length.
    runs = runs_of(dark[:, x])
    ons = [n for on, n in runs if on and n >= width * 0.8]
    offs = [n for on, n in runs if not on and n >= width * 0.6]
    mode = lambda a: int(np.bincount(np.array(a)).argmax())
    return width, mode(ons), mode(offs)

def main(out_dir):
    print("%-30s %9s %8s %12s %12s" %
          ("case", "width_px", "dev_px", "dash/gap", "expected"))
    ok = True
    for name, want, (ed, eg) in CASES:
        width, dash, gap = measure(os.path.join(out_dir, name + ".png"))
        dev = width - want
        ok &= abs(dev) < 0.6
        print("%-30s %9.2f %+8.2f %6d/%-5d %6d/%-5d" %
              (name, width, dev, dash, gap, ed, eg))
    print("RESULT:", "PASS" if ok else "FAIL")
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
