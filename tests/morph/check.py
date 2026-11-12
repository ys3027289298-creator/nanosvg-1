import struct, zlib, sys, os

def load_png(path):
    data = open(path, "rb").read()
    pos, w, h, idat = 8, 0, 0, b""
    while pos < len(data):
        ln, typ = struct.unpack(">I4s", data[pos:pos+8])
        chunk = data[pos+8:pos+8+ln]
        if typ == b"IHDR":
            w, h = struct.unpack(">II", chunk[:8])
        elif typ == b"IDAT": idat += chunk
        pos += 12 + ln
    raw, px = zlib.decompress(idat), bytearray(w * h * 4)
    stride, i = w * 4, 0
    for y in range(h):
        ft = raw[i]; i += 1
        line = bytearray(raw[i:i+stride]); i += stride
        poff = (y - 1) * stride  # index of the previous row inside px
        if ft == 1:
            for x in range(4, stride): line[x] = (line[x] + line[x-4]) & 255
        elif ft == 2:
            for x in range(stride):
                line[x] = (line[x] + (px[poff+x] if y else 0)) & 255
        elif ft == 3:
            for x in range(stride):
                a = line[x-4] if x >= 4 else 0
                up = px[poff+x] if y else 0
                line[x] = (line[x] + ((a + up) >> 1)) & 255
        elif ft == 4:
            for x in range(stride):
                a = line[x-4] if x >= 4 else 0
                b = px[poff+x] if y else 0
                c = px[poff+x-4] if y and x >= 4 else 0
                pp = a + b - c
                pa, pb, pc = abs(pp-a), abs(pp-b), abs(pp-c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[x] = (line[x] + pr) & 255
        px[y*stride:(y+1)*stride] = line
    return w, h, px

def stats(path):
    w, h, px = load_png(path)
    xs, ys, n = [], [], 0
    for i in range(w * h):
        if px[i*4+3] != 0: n += 1; xs.append(i % w); ys.append(i // w)
    box = (min(xs), min(ys), max(xs), max(ys)) if n else (0, 0, 0, 0)
    return n, box

def main(out_dir):
    print("%-12s %10s %26s" % ("case", "pixels", "bbox expand l,r,t,b (px)"))
    for name in ("dilate_r2", "erode_r2", "radius0", "regression"):
        n, box = stats(os.path.join(out_dir, name + ".png"))
        _, base = stats(os.path.join(out_dir, name + "_base.png"))
        dl, dt, dr, db = base[0]-box[0], base[1]-box[1], box[2]-base[2], box[3]-base[3]
        print("%-12s %10d %22s" % (name, n, "(%+d,%+d,%+d,%+d)" % (dl, dr, dt, db)))

if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "output")
