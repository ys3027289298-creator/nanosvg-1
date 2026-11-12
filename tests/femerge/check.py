import struct, zlib, time, sys

def load_png(path):
    d = open(path, 'rb').read()
    pos, w, h, raw = 8, 0, 0, b''
    while pos < len(d):
        n = struct.unpack('>I', d[pos:pos+4])[0]
        typ, body = d[pos+4:pos+8], d[pos+8:pos+8+n]
        if typ == b'IHDR': w, h = struct.unpack('>II', body[:8])
        if typ == b'IDAT': raw += body
        pos += 12 + n
    d, prev, rows, i = zlib.decompress(raw), bytearray(w*4), [], 0
    for y in range(h):
        f = d[i]; i += 1
        line = bytearray(d[i:i+w*4]); i += w*4
        for x in range(w*4):
            a = line[x-4] if x >= 4 else 0
            b, c = prev[x], prev[x-4] if x >= 4 else 0
            if f == 1: line[x] = (line[x]+a)&255
            elif f == 2: line[x] = (line[x]+b)&255
            elif f == 3: line[x] = (line[x]+(a+b)//2)&255
            elif f == 4:
                p = a+b-c; pa, pb, pc = abs(p-a), abs(p-b), abs(p-c)
                line[x] = (line[x]+(a if pa<=pb and pa<=pc else b if pb<=pc else c))&255
        rows.append(line); prev = line
    return w, h, rows

def expect(layers):  # source-over 公式，layers 自下而上，(r,g,b,a) 取 0..255
    pr = pg = pb = pa = 0.0
    for r, g, b, a in layers:
        af = a/255.0
        pr, pg, pb = r*af+pr*(1-af), g*af+pg*(1-af), b*af+pb*(1-af)
        pa = af + pa*(1-af)
    return (round(pr/pa), round(pg/pa), round(pb/pa), round(pa*255)) if pa else (0,0,0,0)

A = 127  # flood-opacity 0.5 -> int(0.5*255)；0.25 -> 63
RED, GRN, BLU, WHT = (255,0,0,A), (0,255,0,A), (0,0,255,A), (255,255,255,63)
cases = [("merge2.png", [RED, BLU]), ("merge4.png", [RED, GRN, BLU, WHT]),
         ("merge_missing.png", [RED, BLU])]
fails = 0
for name, layers in cases:
    w, h, rows = load_png("output/" + name)
    got = tuple(rows[50][200:204])
    exp = expect(layers)
    ok = all(abs(g-e) <= 4 for g, e in zip(got, exp))
    fails += 0 if ok else 1
    print("%s: 采样(50,50)=%s 公式值=%s -> %s" % (name, got, exp, "OK" if ok else "FAIL"))
    time.sleep(2)
w, h, rows = load_png("output/svg.png")
n = sum(1 for row in rows for x in range(w) if row[x*4+3] > 0)
print("svg.png(回归): %dx%d 非透明像素=%d -> %s" % (w, h, n, "OK" if n > 0 else "FAIL"))
fails += 0 if n > 0 else 1
time.sleep(2)
sys.exit(1 if fails else 0)
