import sys, zlib, struct

def read_png(path):
    data = open(path, 'rb').read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n'
    pos, idat, w, h = 8, b'', 0, 0
    while pos < len(data):
        ln, typ = struct.unpack('>I4s', data[pos:pos+8])
        chunk = data[pos+8:pos+8+ln]
        if typ == b'IHDR':
            w, h, bd, ct = struct.unpack('>IIBB', chunk[:10])
            assert bd == 8 and ct == 6
        elif typ == b'IDAT':
            idat += chunk
        pos += 12 + ln
    raw = zlib.decompress(idat)
    stride, px = w * 4, bytearray(w * h * 4)
    prev = bytearray(stride)
    for y in range(h):
        row = bytearray(raw[y*(stride+1)+1:(y+1)*(stride+1)])
        f = raw[y*(stride+1)]
        for i in range(stride):
            a = row[i-4] if i >= 4 else 0
            b, c = prev[i], (prev[i-4] if i >= 4 else 0)
            if f == 1: row[i] = (row[i] + a) & 255
            elif f == 2: row[i] = (row[i] + b) & 255
            elif f == 3: row[i] = (row[i] + (a + b)//2) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p-a), abs(p-b), abs(p-c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                row[i] = (row[i] + pr) & 255
        px[y*stride:(y+1)*stride] = row
        prev = row
    return w, h, px

CASES = [
    ("switch-zh",       "zh分支 rect(10,10,30x30)",   900, (10, 10, 39, 39)),
    ("switch-en",       "en分支 rect(55,55,40x20)",   800, (55, 55, 94, 74)),
    ("switch-fallback", "无命中回退 rect(70,70,20x20)", 400, (70, 70, 89, 89)),
    ("switch-nested",   "嵌套内层 zh-Hans rect(30,30,25x25)", 625, (30, 30, 54, 54)),
]

ok_all = True
for name, expect_desc, expect_n, expect_bb in CASES:
    w, h, px = read_png("output/%s.png" % name)
    xs, ys = [], []
    for y in range(h):
        for x in range(w):
            if px[(y*w + x)*4 + 3] > 0:
                xs.append(x); ys.append(y)
    n = len(xs)
    bb = (min(xs), min(ys), max(xs), max(ys)) if n else None
    ok = (n == expect_n and bb == expect_bb)
    ok_all = ok_all and ok
    print("%-16s 像素数=%-5d 包围盒=%-18s 预期分支: %s -> %s" % (
        name, n, bb, expect_desc, "一致" if ok else "不一致"))
sys.exit(0 if ok_all else 1)
