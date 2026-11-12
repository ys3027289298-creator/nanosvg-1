#!/usr/bin/env python3
# Finds the drop shadow in each rendered case: bounding box and mean alpha.
# Shadow pixels = visible pixels whose color differs from the shape fill.
# Pixels with alpha < 16 are skipped: unpremultiply rounding makes their
# color unreliable (they are antialiasing noise, not shadow).
import re, sys, xml.etree.ElementTree as ET

OUT = "output"
CASES = ["offset-positive", "offset-negative", "big-blur", "regression"]
MIN_ALPHA = 16

def dims(name):
    root = ET.parse(f"tests/dropshadow/{name}.svg").getroot()
    return int(root.get("width")), int(root.get("height"))

def fill_rgb(name):
    text = open(f"tests/dropshadow/{name}.svg").read()
    m = re.search(r'fill="#(..)(..)(..)"', text)
    return tuple(int(v, 16) for v in m.groups())

def analyze(name):
    w, h = dims(name)
    px = open(f"{OUT}/{name}.rgba", "rb").read()
    assert len(px) == w * h * 4, f"{name}: bad rgba size"
    fr, fg, fb = fill_rgb(name)
    xs, ys, alphas = [], [], []
    for y in range(h):
        for x in range(w):
            i = (y * w + x) * 4
            r, g, b, a = px[i], px[i+1], px[i+2], px[i+3]
            if a >= MIN_ALPHA and abs(r-fr) + abs(g-fg) + abs(b-fb) > 12:
                xs.append(x); ys.append(y); alphas.append(a)
    if not xs:
        print(f"[analyze] {name}: no shadow pixels")
        return
    bbox = (min(xs), min(ys), max(xs)+1, max(ys)+1)
    print(f"[analyze] {name}: bbox={bbox} mean_alpha={sum(alphas)/len(alphas):.1f} px={len(alphas)}")

if __name__ == "__main__":
    for case in CASES:
        analyze(case)
