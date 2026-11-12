#!/usr/bin/env python3
import math
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).parent

def render(name):
    raw = subprocess.run([str(ROOT / "output/render"), str(HERE / name)],
                         check=True, capture_output=True).stdout
    w, h = struct.unpack("ii", raw[:8])
    return w, h, raw[8:]


def lut(stops, t):
    """Replicate nsvg__initPaint LUT and the scanline int(t*255) lookup."""
    k = int(min(1.0, max(0.0, t)) * 255.0)
    if len(stops) == 1:
        return round(255 * stops[0][1])
    if k < round(stops[0][0] * 255):
        return round(255 * stops[0][1])
    if k >= round(stops[-1][0] * 255):
        return round(255 * stops[-1][1])
    for (o0, a0), (o1, a1) in zip(stops, stops[1:]):
        ia, ib = round(o0 * 255), round(o1 * 255)
        if ia <= k < ib:
            iu = int((k - ia) / (ib - ia) * 256.0)
            ca, cb = round(255 * a0), round(255 * a1)
            return (ca * (256 - iu) + cb * iu) >> 8
    return round(255 * stops[-1][1])


def report(name, kind, stops):
    w, h, px = render(name)
    print(name)
    y = round((h - 1) / 2.0)
    rgrad = 1.0 if kind == "L" else w / 2.0
    xs = (sorted({round(i * (w - 1) / 15) for i in range(16)}) if kind == "L"
          else [round((w - 1) / 2.0) + i for i in range(16)
                if round((w - 1) / 2.0) + i < w])
    for i, x in enumerate(xs):
        t = x / (w - 1) if kind == "L" else (x - xs[0]) / rgrad
        got = px[(y * w + x) * 4 + 3]
        want = float(lut(stops, t))
        print(f"  i={i:2d} x={x:2d} t={t:.3f} alpha={got:3d} "
              f"theory={want:6.1f} diff={got-want:+.1f}")


CASES = [
    ("linear.svg", "L", [(0.0, 1.0), (1.0, 0.0)]),
    ("radial.svg", "R", [(0.0, 1.0), (1.0, 0.0)]),
    ("single.svg", "L", [(0.5, 1.0)]),
    ("regression.svg", "L", [(0.0, 1.0), (0.5, 0.5), (1.0, 0.0)]),
]
if __name__ == "__main__":
    for name, kind, stops in CASES:
        report(name, kind, stops)
