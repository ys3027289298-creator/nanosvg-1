import math, sys

TOL = 4

def px(d, x, y, w):
    i = (y * w + x) * 4
    return d[i], d[i+1], d[i+2]

def check(name, path, w, y, pts, color):
    d = open(path, 'rb').read()
    worst = 0
    for x, t in pts:
        got = px(d, x, y, w)
        exp = color(min(max(t, 0.0), 1.0))
        dev = max(abs(g - e) for g, e in zip(got, exp))
        worst = max(worst, dev)
        print("  x=%3d t=%.4f got=(%3d,%3d,%3d) exp=(%3d,%3d,%3d) dev=%d" % (
            x, t, got[0], got[1], got[2], exp[0], exp[1], exp[2], dev))
    print("%s: max deviation %d (tol %d) -> %s" % (
        name, worst, TOL, "PASS" if worst <= TOL else "FAIL"))
    return worst <= TOL

c45 = math.cos(math.radians(45))
red_blue = lambda t: (round(255*(1-t)), 0, round(255*t))
gray     = lambda t: (round(255*(1-t)),) * 3
red_grn  = lambda t: (round(255*(1-t)), round(255*t), 0)

ok = True

# Rotated linear (rotate 45): objectBoundingBox left half, userSpaceOnUse right half.
pts = [(x, 0.5 + (x/60.0 - 0.5) * c45) for x in range(5, 56, 5)]
pts += [(x, 0.5 + (x - 90) * c45 / 60.0) for x in range(65, 116, 5)]
ok &= check("rotated-linear", "output/rotated-linear.raw", 120, 30, pts, red_blue)

# Reflected radial: t_raw = |x-50|/25 folded into [0,1].
def reflect(x):
    m = (abs(x - 50) / 25.0) % 2.0
    return 2.0 - m if m > 1.0 else m
pts = [(x, reflect(x)) for x in range(5, 100, 5)]
ok &= check("reflected-radial", "output/reflected-radial.raw", 100, 50, pts, gray)

# Repeated linear: tile every 25 px (12.5 user units, viewBox scaled x2).
pts = [(x, (x / 25.0) % 1.0) for x in range(2, 100, 5)]
ok &= check("repeated-linear", "output/repeated-linear.raw", 100, 30, pts, red_grn)

sys.exit(0 if ok else 1)
