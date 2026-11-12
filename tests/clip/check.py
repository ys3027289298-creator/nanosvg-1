#!/usr/bin/env python3
import os, struct, zlib

ROOT = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(ROOT, "..", "..", "output")

def read_png(path):
    data = open(path, "rb").read()
    pos, w, h, idat = 8, 0, 0, b""
    while pos < len(data):
        ln, kind = struct.unpack(">I4s", data[pos:pos+8]); pos += 8
        chunk = data[pos:pos+ln]; pos += ln + 4
        if kind == b"IHDR":
            w, h = struct.unpack(">II", chunk[:8])
        elif kind == b"IDAT":
            idat += chunk
    raw = zlib.decompress(idat)
    stride, px, prev, pos = w * 4, [], bytearray(w * 4), 0
    def paeth(a, b, c):
        p = a + b - c
        pa, pb, pc = abs(p-a), abs(p-b), abs(p-c)
        return a if pa <= pb and pa <= pc else (b if pb <= pc else c)
    for y in range(h):
        f, row = raw[pos], bytearray(raw[pos+1:pos+1+stride]); pos += stride + 1
        for i in range(stride):
            a = row[i-4] if i >= 4 else 0
            b, c = prev[i], prev[i-4] if i >= 4 else 0
            row[i] = (row[i] + (0 if f == 0 else a if f == 1 else b if f == 2
                                else (a+b)//2 if f == 3 else paeth(a, b, c))) & 0xFF
        prev = row
        px.extend([tuple(row[i:i+4]) for i in range(0, stride, 4)])
    return w, h, px

def inside(case, x, y):
    if case == "rect_clip":
        return 20 <= x < 80 and 20 <= y < 80
    if case == "circle_clip":
        return (x-49.5)**2 + (y-49.5)**2 <= 40.0**2
    if case == "nested_clip":
        d2 = (x-49.5)**2 + (y-49.5)**2
        return 15 <= x < 85 and 15 <= y < 85 and d2 <= 42.0**2
    return True  # missing_ref: missing reference clips nothing

for case in ["rect_clip", "circle_clip", "nested_clip", "missing_ref"]:
    w, h, px = read_png(os.path.join(OUT, case + ".png"))
    out, xs, ys = 0, [], []
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[y*w + x]
            if a > 10 and g > r + 40 and g > b + 20:  # the clipped green shape
                xs.append(x); ys.append(y)
                if not inside(case, x, y):
                    out += 1
    bbox = (min(xs), min(ys), max(xs), max(ys)) if xs else None
    print("%-12s outside_pixels=%d bbox=%s" % (case, out, bbox))
