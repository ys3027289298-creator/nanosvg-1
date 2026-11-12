import sys, zlib, struct

def read_png(path):
    data = open(path, 'rb').read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', 'not a png'
    pos, idat, w, h = 8, b'', 0, 0
    while pos < len(data):
        ln, typ = struct.unpack('>I4s', data[pos:pos + 8])
        if typ == b'IHDR':
            w, h, depth, ctype = struct.unpack('>IIBB', data[pos + 8:pos + 18])
            assert depth == 8 and ctype == 6, 'need 8-bit RGBA png'
        elif typ == b'IDAT':
            idat += data[pos + 8:pos + 8 + ln]
        pos += 12 + ln
    raw, stride = zlib.decompress(idat), w * 4
    img, prev, off = bytearray(w * h * 4), bytearray(stride), 0
    for y in range(h):
        filt = raw[off]; off += 1
        line = bytearray(raw[off:off + stride]); off += stride
        for i in range(stride):
            a = line[i - 4] if i >= 4 else 0
            b, c = prev[i], (prev[i - 4] if i >= 4 else 0)
            if filt == 1: line[i] = (line[i] + a) & 255
            elif filt == 2: line[i] = (line[i] + b) & 255
            elif filt == 3: line[i] = (line[i] + (a + b) // 2) & 255
            elif filt == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        img[y * stride:(y + 1) * stride] = line
        prev = line
    return img, w

# name, x, y, expected RGBA (premultiplied Porter-Duff theory, unpremultiplied)
CASES = [
    ('merge_two_layers', 50, 50, (203, 0, 0, 159)),
    ('composite_over', 50, 50, (0, 85, 0, 191)),
    ('composite_in', 50, 50, (0, 0, 255, 63)),
    ('composite_xor', 30, 30, (0, 0, 0, 0)),
    ('composite_xor', 75, 75, (0, 0, 255, 255)),
]
TOL = 2

def main():
    only = sys.argv[1] if len(sys.argv) > 1 else None
    failed = 0
    for name, x, y, want in CASES:
        if only and name != only:
            continue
        img, w = read_png('output/%s.png' % name)
        got = tuple(img[(y * w + x) * 4:(y * w + x) * 4 + 4])
        ok = all(abs(g - t) <= TOL for g, t in zip(got, want))
        failed += 0 if ok else 1
        print('%-18s (%3d,%3d) 实测=%s 理论=%s %s' % (name, x, y, got, want, 'OK' if ok else 'FAIL'))
    sys.exit(1 if failed else 0)

main()
