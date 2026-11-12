import os, struct, sys

OUT = "output"

def load(path):
    with open(path, "rb") as f:
        data = f.read()
    w, h = struct.unpack("<ii", data[:8])
    return w, h, data[8:]

def alpha_variance(w, h, px, box):
    x0, y0, x1, y1 = box
    vals = [px[(y * w + x) * 4 + 3] for y in range(y0, y1) for x in range(x0, x1)]
    mean = sum(vals) / len(vals)
    return sum((v - mean) ** 2 for v in vals) / len(vals)

def grain_size(w, h, px, box):
    x0, y0, x1, y1 = box
    runs = total = 0
    for y in range(y0, y1):
        row = [sum(px[(y * w + x) * 4:(y * w + x) * 4 + 3]) for x in range(x0, x1)]
        mean = sum(row) / len(row)
        above = row[0] >= mean
        for v in row[1:]:
            cur = v >= mean
            runs += cur != above
            above = cur
        total += len(row)
    return total / runs if runs else 0.0

# name, alpha box, grain box, alpha var range, grain size range
CASES = [
    ("kernel-unit",    (0, 0, 128, 128), (4, 4, 124, 124), (100.0, 1e9), (2.0, 16.0)),
    ("kernel-unit-hi", (0, 0, 256, 256), (8, 8, 248, 248), (100.0, 1e9), (4.0, 32.0)),
    ("preserve-alpha", (0, 0, 128, 16),  (34, 34, 94, 94), (0.0, 0.0),   (1.5, 10.0)),
    ("combo",          (0, 0, 128, 16),  (34, 34, 94, 94), (0.0, 0.0),   (2.0, 14.0)),
]

def main():
    ok = True
    print("%-16s %14s %12s %s" % ("case", "alpha-var", "grain-size", "result"))
    for name, abox, gbox, arng, grng in CASES:
        w, h, px = load(os.path.join(OUT, name + ".rgba"))
        avar = alpha_variance(w, h, px, abox)
        size = grain_size(w, h, px, gbox)
        good = arng[0] <= avar <= arng[1] and grng[0] <= size <= grng[1]
        ok &= good
        print("%-16s %14.2f %12.2f %s" % (name, avar, size, "PASS" if good else "FAIL"))
    w, h, cur = load(os.path.join(OUT, "regression.rgba"))
    _, _, ref = load(os.path.join(OUT, "ref", "regression.rgba"))
    avar = alpha_variance(w, h, cur, (0, 0, w, h))
    good = cur == ref
    ok &= good
    print("%-16s %14.2f %12s %s" % ("regression", avar, "n/a", "PASS" if good else "FAIL"))
    print("overall: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1

sys.exit(main())
