#!/usr/bin/env python3
"""Validate feDiffuseLighting output against a reference implementation.

The reference mirrors the library code in src/nanosvg.h one-to-one:
  * height field  h(x,y) = alpha(x,y) / 255
  * normals from central differences of surfaceScale * h (edges clamped)
  * feDistantLight / fePointLight per SVG 1.1
  * intensity    = diffuseConstant * max(dot(N, L), 0)
  * output color = input_color * lighting_color * intensity (clamped)

For every SVG, 16 sample points on a fixed 4x4 lattice are compared and the
per-channel luminance deviation is printed.
"""

import math
import os
import struct
import sys
import xml.etree.ElementTree as ET


W = H = 64
N_SAMPLES = 16


def load_rgba(path):
    with open(path, "rb") as fp:
        data = fp.read()
    assert len(data) == W * H * 4, "unexpected raw RGBA size"
    px = [list(data[i:i + 4]) for i in range(0, len(data), 4)]
    return px


def parse_svg(path):
    """Return the parsed feDiffuseLighting parameters for the first filter."""
    ns = {"s": "http://www.w3.org/2000/svg"}
    tree = ET.parse(path)
    root = tree.getroot()
    node = root.find(".//s:feDiffuseLighting", ns)
    if node is None:
        return None

    def fnum(name, default):
        value = node.get(name)
        return float(value) if value is not None else default

    color = node.get("lighting-color", "#ffffff").lstrip("#")
    r = int(color[0:2], 16)
    g = int(color[2:4], 16)
    b = int(color[4:6], 16)

    distant = node.find("s:feDistantLight", ns)
    point = node.find("s:fePointLight", ns)
    if distant is not None:
        return {
            "type": "distant",
            "surfaceScale": fnum("surfaceScale", 1.0),
            "diffuseConstant": fnum("diffuseConstant", 1.0),
            "color": (r, g, b),
            "azimuth": math.radians(float(distant.get("azimuth", "0"))),
            "elevation": math.radians(float(distant.get("elevation", "0"))),
        }
    return {
        "type": "point",
        "surfaceScale": fnum("surfaceScale", 1.0),
        "diffuseConstant": fnum("diffuseConstant", 1.0),
        "color": (r, g, b),
        "x": float(point.get("x", "0")),
        "y": float(point.get("y", "0")),
        "z": float(point.get("z", "0")),
    }


def reference_color(base, light, x, y):
    """Expected lit RGBA at (x, y) using the same math as the C implementation.

    'base' is the unlit rasterization: its alpha defines the bump surface and
    its RGB is the input color the light intensity is multiplied onto.
    """
    r0, g0, b0, a0 = base[y * W + x]
    if a0 == 0 or light is None:
        return tuple(base[y * W + x])

    def height(xx, yy):
        cx = min(max(xx, 0), W - 1)
        cy = min(max(yy, 0), H - 1)
        return base[cy * W + cx][3] / 255.0

    ss = light["surfaceScale"]
    xl = min(max(x - 1, 0), W - 1)
    xr = min(max(x + 1, 0), W - 1)
    yt = min(max(y - 1, 0), H - 1)
    yb = min(max(y + 1, 0), H - 1)
    hl, hr = base[y * W + xl][3] / 255.0 * ss, base[y * W + xr][3] / 255.0 * ss
    ht, hb = base[yt * W + x][3] / 255.0 * ss, base[yb * W + x][3] / 255.0 * ss
    nx, ny, nz = (hr - hl) * 0.5, (hb - ht) * 0.5, 1.0
    nlen = math.sqrt(nx * nx + ny * ny + nz * nz)
    nx, ny, nz = nx / nlen, ny / nlen, nz / nlen

    if light["type"] == "distant":
        lx = math.cos(light["elevation"]) * math.cos(light["azimuth"])
        ly = math.cos(light["elevation"]) * math.sin(light["azimuth"])
        lz = math.sin(light["elevation"])
    else:
        lx, ly, lz = light["x"] - x, light["y"] - y, light["z"]
        llen = math.sqrt(lx * lx + ly * ly + lz * lz)
        lx, ly, lz = lx / llen, ly / llen, lz / llen

    intensity = light["diffuseConstant"] * max(nx * lx + ny * ly + nz * lz, 0.0)
    lr, lg, lb = (c / 255.0 for c in light["color"])
    out = [
        min(255.0, r0 * lr * intensity),
        min(255.0, g0 * lg * intensity),
        min(255.0, b0 * lb * intensity),
        a0,
    ]
    return tuple(out)


def luminance(c):
    return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2]


def check_case(svg_path, rgba_path, base_path):
    light = parse_svg(svg_path)
    px = load_rgba(rgba_path)
    base = load_rgba(base_path)
    name = os.path.basename(svg_path)
    print("\n=== %s ===" % name)
    if light is None:
        print("  no lighting filter (regression sample)")
        diff = sum(abs(a - b) for ra, rb in zip(px, base) for a, b in zip(ra, rb))
        print("  cumulative byte difference vs baseline: %d" % diff)
        return float(diff), (1 if diff else 0)

    max_dev = 0.0
    bad = 0
    print("  sample   x   y   measured Y  expected Y   abs dev   pct dev")
    for i in range(N_SAMPLES):
        col = i % 4
        row = i // 4
        x = 8 + col * 16      # 8, 24, 40, 56
        y = 8 + row * 16
        actual = px[y * W + x]
        expected = reference_color(base, light, x, y)
        ym = luminance(actual)
        ye = luminance(expected)
        dev = abs(ym - ye)
        pct = (100.0 * dev / ye) if ye > 1e-6 else 0.0
        max_dev = max(max_dev, dev)
        if dev > 1.5:
            bad += 1
        print("  %6d  %3d %3d   %10.4f  %10.4f  %9.4f  %8.3f%%"
              % (i, x, y, ym, ye, dev, pct))
    print("  max luminance deviation: %.4f / 255 (%.4f%%)"
          % (max_dev, 100.0 * max_dev / 255.0))
    return max_dev, bad


def main():
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    out_dir = os.path.join(root, "output")
    base_dir = os.path.join(out_dir, "baseline")
    cases = ["distant", "point", "bigbump", "regression"]
    overall_max = 0.0
    total_bad = 0
    for case in cases:
        svg = os.path.join(root, "tests", "lighting", case + ".svg")
        rgba = os.path.join(out_dir, case + ".rgba")
        base_rgba = os.path.join(base_dir, case + ".rgba")
        dev, bad = check_case(svg, rgba, base_rgba)
        overall_max = max(overall_max, dev)
        total_bad += bad
    print("\n=== summary ===")
    print("overall max luminance deviation: %.4f / 255" % overall_max)
    print("samples over 1.5/255 tolerance: %d" % total_bad)
    sys.exit(1 if total_bad else 0)


if __name__ == "__main__":
    main()
