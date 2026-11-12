#!/usr/bin/env python3
"""Measure lit-region radius in the rendered feSpotLight PNGs and compare
against the theoretical cone/plane intersection.

Usage: python3 measure.py            (run from output/)

No third-party dependencies: PNGs are decoded with zlib/struct only.
"""

import math
import struct
import sys
import zlib

W = H = 400
THRESHOLD = 4  # 8-bit luminance above this counts as lit


def read_png(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")
    pos = 8
    width = height = None
    bitdepth = colortype = None
    idat = b""
    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos:pos + 4])
        ctype = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + length]
        if ctype == b"IHDR":
            width, height, bitdepth, colortype = struct.unpack(">IIBB", body[:10])
        elif ctype == b"IDAT":
            idat += body
        elif ctype == b"IEND":
            break
        pos += 12 + length
    if bitdepth != 8 or colortype not in (0, 2, 4, 6):
        raise ValueError(f"{path}: unsupported format depth={bitdepth} type={colortype}")
    channels = {0: 1, 2: 3, 4: 2, 6: 4}[colortype]
    stride = width * channels
    raw = zlib.decompress(idat)
    out = bytearray(width * height * channels)
    prev = bytearray(stride)
    src = 0
    for y in range(height):
        ftype = raw[src]
        src += 1
        row = bytearray(raw[src:src + stride])
        src += stride
        if ftype == 1:  # Sub
            for i in range(channels, stride):
                row[i] = (row[i] + row[i - channels]) & 0xFF
        elif ftype == 2:  # Up
            for i in range(stride):
                row[i] = (row[i] + prev[i]) & 0xFF
        elif ftype == 3:  # Average
            for i in range(stride):
                a = row[i - channels] if i >= channels else 0
                row[i] = (row[i] + ((a + prev[i]) >> 1)) & 0xFF
        elif ftype == 4:  # Paeth
            for i in range(stride):
                a = row[i - channels] if i >= channels else 0
                b = prev[i]
                c = prev[i - channels] if i >= channels else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                row[i] = (row[i] + pr) & 0xFF
        out[y * stride:(y + 1) * stride] = row
        prev = row
    # return luminance plane (row-major, pixel centers at i+0.5)
    lum = []
    for y in range(height):
        base = y * stride
        lum.append([out[base + x * channels] for x in range(width)])
    return lum


def lit_mask(lum):
    pts = []
    for y in range(H):
        row = lum[y]
        for x in range(W):
            if row[x] > THRESHOLD:
                pts.append((x + 0.5, y + 0.5))
    return pts


def cone_ellipse(px, py, pz, ax, ay, az, cone_deg):
    """Theoretical intersection of the spot cone with plane z=0.

    Returns (cx, cy, a, b, t) where (cx, cy) is the ellipse center, a/b the
    semi-axes and t the unit direction of the major axis in the plane.
    """
    sx, sy, sz = ax - px, ay - py, az - pz
    n = math.sqrt(sx * sx + sy * sy + sz * sz)
    sx, sy, sz = sx / n, sy / n, sz / n
    cos_a = math.cos(math.radians(cone_deg))
    cos2 = cos_a * cos_a

    horiz = math.hypot(sx, sy)
    if horiz < 1e-12:
        tx, ty = 1.0, 0.0  # axis perpendicular to plane: any direction works
    else:
        tx, ty = sx / horiz, sy / horiz
    nx, ny = -ty, tx

    # Quadratic along the major-axis line through (px, py):
    # (u*A - pz*sz)^2 = (u^2 + pz^2) * cos2,  A = t . s
    A = tx * sx + ty * sy
    qa = A * A - cos2
    qb = -2.0 * A * pz * sz
    qc = pz * pz * (sz * sz - cos2)
    disc = qb * qb - 4 * qa * qc
    if disc <= 0:
        return None
    sq = math.sqrt(disc)
    u1 = (-qb - sq) / (2 * qa)
    u2 = (-qb + sq) / (2 * qa)
    u0 = (u1 + u2) / 2.0
    a = abs(u2 - u1) / 2.0
    cx, cy = px + u0 * tx, py + u0 * ty

    # Quadratic along the perpendicular direction at u = u0:
    # (v*B + C)^2 = (v^2 + u0^2 + pz^2) * cos2,  B = n . s, C = u0*A - pz*sz
    B = nx * sx + ny * sy
    C = u0 * A - pz * sz
    qa2 = B * B - cos2
    qb2 = 2.0 * B * C
    qc2 = C * C - (u0 * u0 + pz * pz) * cos2
    disc2 = qb2 * qb2 - 4 * qa2 * qc2
    if disc2 <= 0:
        return None
    b = math.sqrt(disc2) / abs(2 * qa2)
    return (cx, cy, a, b, (tx, ty))


def measure(pts, axis):
    """Measured semi-axes: max |projection| of lit pixels onto axis/normal."""
    mx = sum(p[0] for p in pts) / len(pts)
    my = sum(p[1] for p in pts) / len(pts)
    tx, ty = axis
    nx, ny = -ty, tx
    a = b = 0.0
    for x, y in pts:
        dx, dy = x - mx, y - my
        a = max(a, abs(dx * tx + dy * ty))
        b = max(b, abs(dx * nx + dy * ny))
    rmax = max(math.hypot(x - mx, y - my) for x, y in pts)
    return (mx, my), a, b, rmax


def check(name, meas, theory, tol_px=1.5, tol_frac=0.03):
    err = abs(meas - theory)
    ok = err <= max(tol_px, tol_frac * theory)
    print(f"    {name:<14} measured={meas:8.2f}px  theoretical={theory:8.2f}px  "
          f"err={err:5.2f}px  {'OK' if ok else 'FAIL'}")
    return ok


def falloff_check(lum, cx, cy, rmax, light, cone_deg, exp):
    """Compare measured radial falloff (diffuse) with the closed form:
    I(r) ~ (N.L) * (L.S)^specularExponent."""
    px, py, pz, ax, ay, az = light
    print(f"    falloff along +x (diffuse, specularExponent={exp:g}):")
    ok = True
    for frac in (0.25, 0.5, 0.75, 0.95):
        r = frac * rmax
        x = int(round(cx + r - 0.5))
        y = int(round(cy - 0.5))
        meas = lum[y][x] / max(1, lum[int(round(cy - 0.5))][int(round(cx - 0.5))])
        qx, qy = cx + r, cy
        L = [px - qx, py - qy, pz]
        n = math.sqrt(sum(v * v for v in L))
        L = [v / n for v in L]
        S = [ax - px, ay - py, az - pz]
        m = math.sqrt(sum(v * v for v in S))
        S = [v / m for v in S]
        dot = sum(L[i] * S[i] for i in range(3))
        ndotl = L[2]
        theo = (ndotl * dot ** exp) / 1.0  # at center: ndotl=dot=1
        err = abs(meas - theo)
        good = err <= 0.03
        ok = ok and good
        print(f"      r={r:6.1f}px ({frac:.2f}*rmax)  measured={meas:.4f}  "
              f"theoretical={theo:.4f}  {'OK' if good else 'FAIL'}")
    return ok


CASES = [
    # name, light (x,y,z,pointsAtX,pointsAtY,pointsAtZ), cone deg or None, spot exp
    ("spot_front",      (200, 200, 100, 200, 200, 0), 30.0, 1.0),
    ("spot_oblique",    (200, 200, 100, 260, 200, 0), 20.0, 1.0),
    ("spot_narrow",     (200, 200, 150, 200, 200, 0), 10.0, 4.0),
    ("spot_regression", (200, 200, 120, 200, 200, 0), None, 1.0),
]


def main():
    all_ok = True
    for name, light, cone, exp in CASES:
        print(f"[{name}]  light={light[:3]} pointsAt={light[3:]} "
              f"cone={cone if cone is not None else 'none'} exp={exp}")
        for prim in ("diffuse", "specular"):
            lum = read_png(f"{name}_{prim}.png")
            pts = lit_mask(lum)
            coverage = len(pts) / (W * H)
            if cone is None:
                # Regression: no limitingConeAngle -> whole plane lit.
                (mx, my), a, b, rmax = measure(pts, (1.0, 0.0))
                corner = math.hypot(max(mx, W - mx), max(my, H - my))
                ok = coverage > 0.999
                all_ok &= ok
                print(f"  {prim}: coverage={coverage * 100:.2f}%  "
                      f"measured radius={rmax:.2f}px  theoretical=unbounded "
                      f"(full image, corner={corner:.2f}px)  {'OK' if ok else 'FAIL'}")
                continue
            theo = cone_ellipse(*light, cone)
            if theo is None:
                print(f"  {prim}: theoretical cone misses the plane!")
                all_ok = False
                continue
            cx, cy, ta, tb, axis = theo
            (mx, my), ma, mb, rmax = measure(pts, axis)
            print(f"  {prim}: center measured=({mx:.1f},{my:.1f}) "
                  f"theoretical=({cx:.1f},{cy:.1f})")
            ok = check("semi-axis a", ma, ta)
            ok &= check("semi-axis b", mb, tb)
            if abs(ta - tb) < 1e-6:
                ok &= check("radius", rmax, ta)
            all_ok &= ok
            if name == "spot_narrow" and prim == "diffuse":
                all_ok &= falloff_check(lum, cx, cy, ta, light, cone, exp)
    print()
    print("RESULT:", "PASS" if all_ok else "FAIL")
    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main())
