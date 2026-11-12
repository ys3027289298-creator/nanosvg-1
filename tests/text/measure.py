#!/usr/bin/env python3
"""Measure rendered text width and its deviation from the SVG target.

The cases place text at x=10; the dark-pixel extent (excluding the white
background) gives the actual rendered ink width in pixels.
"""
import os
import sys

from PIL import Image
import numpy as np

# name -> (target textLength in px, -1 means natural layout / no textLength)
CASES = [
    ("01_expand_spacing",        240),
    ("02_condense_spacing",       60),
    ("03_expand_glyphs",         240),
    ("04_regression_plain",       -1),
    ("05_fallback_adjust_only",   -1),
    ("06_fallback_length_only",  240),
]
X0 = 10
TOL = 1.0

def ink_width(png):
    dark = np.asarray(Image.open(png).convert("L")) < 128
    cols = np.where(dark.any(axis=0))[0]
    return int(cols.min()), int(cols.max())

def main(out_dir):
    print("%-28s %8s %10s %10s %9s" %
          ("case", "target", "actual", "deviation", "verdict"))
    ok = True
    for name, target in CASES:
        xmin, xmax = ink_width(os.path.join(out_dir, name + ".png"))
        actual = xmax - xmin + 1
        if target < 0:
            dev, want = 0.0, actual  # natural layout: record width, no target
            verdict = "ok"
        else:
            dev = actual - target
            verdict = "PASS" if abs(dev) <= TOL else "FAIL"
            ok &= abs(dev) <= TOL
        print("%-28s %8s %10d %+9.1fpx %9s" %
              (name, "natural" if target < 0 else target, actual, dev, verdict))
    print("natural baseline 04 == 05:", _same_natural(out_dir))
    print("RESULT:", "PASS" if ok else "FAIL")
    return 0 if ok else 1

def _same_natural(out_dir):
    a = ink_width(os.path.join(out_dir, "04_regression_plain.png"))
    b = ink_width(os.path.join(out_dir, "05_fallback_adjust_only.png"))
    return (b[1] - b[0]) == (a[1] - a[0])

if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
