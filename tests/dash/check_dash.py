import zlib, struct, math, sys

def load_png(p):
    d = open(p, 'rb').read(); i = 8; w = h = 0; idat = b''
    while i < len(d):
        n = struct.unpack('>I', d[i:i+4])[0]; t = d[i+4:i+8]
        if t == b'IHDR': w, h = struct.unpack('>II', d[i+8:i+16])
        if t == b'IDAT': idat += d[i+8:i+8+n]
        i += 12 + n
    raw = zlib.decompress(idat); s = w*4+1; px = bytearray(w*h*4); prev = bytearray(w*4)
    for y in range(h):
        f = raw[y*s]; row = bytearray(raw[y*s+1:(y+1)*s])
        for k in range(w*4):
            a = row[k-4] if k > 3 else 0; b = prev[k]; c = prev[k-4] if k > 3 else 0
            if f == 1: row[k] = (row[k]+a) & 255
            elif f == 2: row[k] = (row[k]+b) & 255
            elif f == 3: row[k] = (row[k]+(a+b)//2) & 255
            elif f == 4:
                q = a+b-c; pa, pb, pc = abs(q-a), abs(q-b), abs(q-c)
                row[k] = (row[k]+(a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        px[y*w*4:(y+1)*w*4] = row; prev = row
    return w, h, px

def bez(p, t):
    u = 1-t
    return (u**3*p[0]+3*u*u*t*p[2]+3*u*t*t*p[4]+t**3*p[6],
            u**3*p[1]+3*u*u*t*p[3]+3*u*t*t*p[5]+t**3*p[7])

N = 2000
CASES = [  # name, dasharray, dashoffset, centerline point generator
    ('line',   [40, 20], 0,  lambda: [(20+360*i/N, 30) for i in range(N+1)]),
    ('circle', [60, 30], 0,  lambda: [(200+150*math.cos(2*math.pi*i/N),
                                       200+150*math.sin(2*math.pi*i/N)) for i in range(N+1)]),
    ('bezier', [45, 15], 0,  lambda: [bez([20,200,120,20,280,20,380,200], i/N) for i in range(N+1)]),
    ('offset', [40, 20], 25, lambda: [(20+360*i/N, 30) for i in range(N+1)]),
]

def on_at(s, seq, per, off):
    s = (s+off) % per; acc = 0.0
    for k, v in enumerate(seq):
        if s < acc+v: return k % 2 == 0
        acc += v
    return True

ok = True
for name, dash, off, gen in CASES:
    w, h, px = load_png('output/%s.png' % name)
    pts = gen(); seq = dash*2 if len(dash) % 2 else dash; per = sum(seq)
    pos = [0.0]
    for i in range(1, len(pts)):
        pos.append(pos[-1]+math.hypot(pts[i][0]-pts[i-1][0], pts[i][1]-pts[i-1][1]))
    got_on = match = 0
    for i, (x, y) in enumerate(pts):
        got = px[((int(round(y))*w)+int(round(x)))*4+3] > 128
        got_on += got; match += (got == on_at(pos[i], seq, per, off))
    n = len(pts); md = got_on/n; td = sum(dash[::2])/per
    print('%-7s duty: measured %.3f theory %.3f dev %+.4f | pattern match %.1f%%'
          % (name, md, td, md-td, 100.0*match/n))
    ok &= abs(md-td) < 0.05 and match/n > 0.95
sys.exit(0 if ok else 1)
