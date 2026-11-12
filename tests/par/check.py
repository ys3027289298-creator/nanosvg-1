import struct, zlib, sys, os

def read_png(path):
    data = open(path, 'rb').read()
    pos, width, height, idat = 8, 0, 0, b''
    while pos < len(data):
        ln, typ = struct.unpack('>I4s', data[pos:pos+8])
        chunk = data[pos+8:pos+8+ln]
        if typ == b'IHDR':
            width, height = struct.unpack('>II', chunk[:8])
        elif typ == b'IDAT':
            idat += chunk
        pos += 12 + ln
    raw = zlib.decompress(idat)
    rows, prev = [], bytearray(width * 4)
    stride = width * 4
    for y in range(height):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - 4] if i >= 4 else 0
            b, c = prev[i], prev[i - 4] if i >= 4 else 0
            p = a + b - c
            pc = abs(p - c)
            paeth = a if abs(p - a) <= abs(p - b) and abs(p - a) <= pc else b if abs(p - b) <= pc else c
            pred = a if f == 1 else b if f == 2 else (a + b) // 2 if f == 3 else paeth if f == 4 else 0
            line[i] = (line[i] + pred) & 255
        prev = line
        rows.append(line)
    return width, height, rows

def bbox(rows):
    xs, ys = [], []
    for y, row in enumerate(rows):
        for x in range(len(row) // 4):
            if row[x * 4 + 3] > 8:
                xs.append(x); ys.append(y)
    return (min(xs), min(ys), max(xs), max(ys)) if xs else None

cases = {
    'par_none.svg':    (0, 0, 99, 99),
    'par_meet.svg':    (0, 20, 99, 79),
    'par_slice.svg':   (0, 0, 99, 99),
    'par_default.svg': (0, 0, 99, 59),
}
out = sys.argv[1]
for name, theory in cases.items():
    stem = name[:-4]
    w, h, rows = read_png(os.path.join(out, stem + '.png'))
    box = bbox(rows)
    dev = tuple(box[i] - theory[i] for i in range(4)) if box else None
    print(f"{name}: bbox={box} theory={theory} deviation={dev}")

base = os.path.join(out, 'baseline', 'par_default.png')
new = os.path.join(out, 'par_default.png')
if os.path.exists(base):
    _, _, rb = read_png(base)
    _, _, rn = read_png(new)
    diff = sum(a != b for a, b in zip(b''.join(rb), b''.join(rn)))
    print(f"regression pixel diff (no attribute, old vs new): {diff}")
