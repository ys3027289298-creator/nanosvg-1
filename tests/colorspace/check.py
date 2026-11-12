#!/usr/bin/env python3
# Samples grayscale across blurred edges and compares against the theoretical
# Gaussian step response (erf curve), in sRGB resp. linear-light space.
import zlib, struct, math, sys

def load_png(path):  # minimal decoder for 8-bit RGBA PNGs (stb_image_write)
    data = open(path, 'rb').read()
    pos, w, h, idat = 8, 0, 0, b''
    while pos < len(data):
        ln, = struct.unpack('>I', data[pos:pos+4]); typ = data[pos+4:pos+8]
        if typ == b'IHDR': w, h = struct.unpack('>II', data[pos+8:pos+16])
        elif typ == b'IDAT': idat += data[pos+8:pos+8+ln]
        pos += 12 + ln
    raw, stride = zlib.decompress(idat), w * 4
    img, prev, p = bytearray(h * stride), bytearray(stride), 0
    for y in range(h):
        f = raw[p]; p += 1
        line = bytearray(raw[p:p+stride]); p += stride
        for i in range(stride):
            a = line[i-4] if i >= 4 else 0; b = prev[i]; c = prev[i-4] if i >= 4 else 0
            if f == 1: line[i] = (line[i] + a) & 255
            elif f == 2: line[i] = (line[i] + b) & 255
            elif f == 3: line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                q = a + b - c; pa, pb, pc = abs(q-a), abs(q-b), abs(q-c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else (b if pb <= pc else c))) & 255
        img[y*stride:(y+1)*stride] = line; prev = line
    return w, img

X0, SIGMA = 31.9, 4.0          # gradient hard-edge position, feGaussianBlur sigma
XS = [24, 26, 28, 30, 32, 34, 36, 38, 40]
def enc(c): return 12.92 * c if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055
def theory(x, linear):
    v = 0.5 * (1 + math.erf((x + 0.5 - X0) / (SIGMA * math.sqrt(2))))
    return 255 * (enc(v) if linear else v)

def run(case, png, row, linear, tol=4.0):
    w, img = load_png(png); worst = 0.0
    print('[%s] %s row=%d' % (case, png, row))
    for x in XS:
        g, t = img[(row * w + x) * 4], theory(x, linear)
        worst = max(worst, abs(g - t))
        print('  x=%2d gray=%3d theory=%6.1f dev=%+5.1f' % (x, g, t, g - t))
    print('  max|dev|=%.1f tol=%.1f -> %s' % (worst, tol, 'PASS' if worst <= tol else 'FAIL'))
    return worst <= tol

ok = run('srgb_blur', 'output/srgb_blur.png', 32, False)
ok &= run('linear_blur', 'output/linear_blur.png', 32, True)
ok &= run('compare/sRGB', 'output/compare.png', 16, False)
ok &= run('compare/linearRGB', 'output/compare.png', 48, True)
w, img = load_png('output/regression.png')   # no filter attr: edge must stay sharp
worst = max(abs(img[(32 * w + x) * 4] - (0 if x + 0.5 < X0 else 255)) for x in XS)
print('[regression] sharp edge max|dev|=%d tol=3 -> %s' % (worst, 'PASS' if worst <= 3 else 'FAIL'))
ok &= worst <= 3
sys.exit(0 if ok else 1)
