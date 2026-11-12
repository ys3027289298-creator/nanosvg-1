#!/usr/bin/env python3
# Samples the fill/stroke overlap region of each rendered test and checks
# which paint ended up on top. Run from the repo root after run.sh renders.
import sys

W = H = 100
FILL = (255, 200, 0)
STROKE = (200, 0, 0)


def load(path):
    with open(path, "rb") as f:
        return f.read()


def px(img, x, y):
    o = (y * W + x) * 4
    return tuple(img[o:o + 3])


def classify(c):
    df = sum((a - b) ** 2 for a, b in zip(c, FILL))
    ds = sum((a - b) ** 2 for a, b in zip(c, STROKE))
    return "fill" if df < ds else "stroke"


# name -> (sample point in the fill/stroke overlap, expected top paint)
cases = [
    ("fill_first", (25, 50), "stroke"),   # explicit fill first
    ("stroke_first", (25, 50), "fill"),   # style: stroke first, wins over attr
    ("partial", (25, 50), "fill"),        # paint-order="stroke" => stroke,fill,markers
    ("default", (12, 25), "stroke"),      # no paint-order => default order
]

ok = True
for name, (x, y), expect in cases:
    c = px(load(f"output/{name}.rgba"), x, y)
    got = classify(c)
    ok &= got == expect
    print(f"[{'OK' if got == expect else 'FAIL'}] {name}: overlap ({x},{y}) "
          f"rgb={c} -> {got} (expect {expect})")

# sanity: fill-only center and stroke-only outer band are stable across orders
img = load("output/fill_first.rgba")
for label, pt, expect in [("center", (50, 50), "fill"), ("outer", (15, 50), "stroke")]:
    c = px(img, *pt)
    got = classify(c)
    ok &= got == expect
    print(f"[{'OK' if got == expect else 'FAIL'}] sanity {label} {pt} rgb={c} -> {got}")

sys.exit(0 if ok else 1)
