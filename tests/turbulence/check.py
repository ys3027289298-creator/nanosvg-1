"""feTurbulence validation: renders test SVGs via example2; checks stats + determinism."""
import os, shutil, struct, subprocess, sys, time, zlib
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")); OUT = os.path.join(ROOT, "output")
SVGS = ["single-octave", "four-octaves", "seeds", "regression"]

def stage(msg):
    print("\n=== %s ===" % msg, flush=True)
    time.sleep(3)

def load_png(path):  # minimal 8-bit RGBA PNG decoder (stdlib only)
    data, pos, w, h, idat = open(path, "rb").read(), 8, 0, 0, b""
    while pos < len(data):
        ln = struct.unpack(">I", data[pos:pos + 4])[0]; typ, chunk = data[pos + 4:pos + 8], data[pos + 8:pos + 8 + ln]
        if typ == b"IHDR": w, h = struct.unpack(">II", chunk[:8])
        elif typ == b"IDAT": idat += chunk
        pos += 12 + ln
    raw, stride, prev, out, i = zlib.decompress(idat), w * 4, bytearray(w * 4), bytearray(), 0
    for _ in range(h):
        f = raw[i]; i += 1; line = bytearray(raw[i:i + stride]); i += stride
        for x in range(stride):
            a, b, c = (line[x - 4] if x >= 4 else 0), prev[x], (prev[x - 4] if x >= 4 else 0)
            if f == 1: line[x] = (line[x] + a) & 255
            elif f == 2: line[x] = (line[x] + b) & 255
            elif f == 3: line[x] = (line[x] + (a + b) // 2) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else (b if pb <= pc else c))) & 255
        out += line; prev = line
    return w, h, out

def render(binpath, svg, png):
    shutil.copy(os.path.join(ROOT, "tests/turbulence", svg + ".svg"), os.path.join(OUT, "example/23.svg"))
    subprocess.run([binpath], cwd=os.path.join(OUT, "run"), check=True, capture_output=True)
    shutil.move(os.path.join(OUT, "run/svg.png"), os.path.join(OUT, png))

for d in ("run", "example"): os.makedirs(os.path.join(OUT, d), exist_ok=True)
stage("Stage 1/7: build example2 renderer")
subprocess.run(["cc", "-O2", "-o", (binpath := os.path.join(OUT, "render")), os.path.join(ROOT, "example/example2.c"),
                "-I" + os.path.join(ROOT, "src"), "-I" + os.path.join(ROOT, "example"), "-lm"], check=True)
for name in SVGS:
    stage("Stage 2/7: render %s.svg" % name)
    render(binpath, name, name + ".png")
for name in SVGS:
    stage("Stage 3/7: stats for %s" % name)
    w, h, px = load_png(os.path.join(OUT, name + ".png")); mean = sum(px) / len(px)
    var = sum((v - mean) ** 2 for v in px) / len(px)
    print("%-14s %dx%d mean=%.2f var=%.2f" % (name, w, h, mean, var), flush=True)
    if var < 1.0: sys.exit("FAIL: %s looks blank" % name)
stage("Stage 4/7: different seeds produce different pixels")
w, h, px = load_png(os.path.join(OUT, "seeds.png")); rb = w * 4
print("left/right half byte mismatches: %d" % (mism := sum(1 for y in range(h) for x in range(rb // 2) if px[y * rb + x] != px[y * rb + rb // 2 + x])), flush=True)
if mism == 0: sys.exit("FAIL: seed has no effect")
stage("Stage 5/7: re-render seeds.svg (same seed)")
render(binpath, "seeds", "seeds_again.png")
stage("Stage 6/7: per-pixel determinism diff")
a = load_png(os.path.join(OUT, "seeds.png"))[2]; b = load_png(os.path.join(OUT, "seeds_again.png"))[2]
print("pixel byte differences: %d" % (diff := sum(1 for x, y in zip(a, b) if x != y)), flush=True)
if diff != 0: sys.exit("FAIL: non-deterministic output")
stage("Stage 7/7: summary")
print("OK: all checks passed; PNGs in %s" % OUT)
