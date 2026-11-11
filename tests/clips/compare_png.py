#!/usr/bin/env python3
"""Compare two 8-bit non-interlaced PNGs without external dependencies."""

import argparse
import struct
import sys
import zlib


def read_png(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG file")

    pos = 8
    idat = bytearray()
    width = height = color_type = bit_depth = interlace = None
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        payload = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            width, height, bit_depth, color_type, _, _, interlace = struct.unpack(">IIBBBBB", payload)
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            break

    if bit_depth != 8 or interlace != 0 or color_type not in (0, 2, 6):
        raise ValueError(f"{path}: unsupported PNG (depth={bit_depth}, type={color_type}, interlace={interlace})")
    channels = {0: 1, 2: 3, 6: 4}[color_type]
    stride = width * channels
    raw = zlib.decompress(bytes(idat))
    out = bytearray(width * height * 4)
    prev = bytearray(stride)
    src = 0

    for y in range(height):
        filt = raw[src]
        src += 1
        row = bytearray(raw[src:src + stride])
        src += stride
        for i in range(stride):
            a = row[i - channels] if i >= channels else 0
            b = prev[i]
            c = prev[i - channels] if i >= channels else 0
            if filt == 1:
                row[i] = (row[i] + a) & 0xff
            elif filt == 2:
                row[i] = (row[i] + b) & 0xff
            elif filt == 3:
                row[i] = (row[i] + ((a + b) >> 1)) & 0xff
            elif filt == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                row[i] = (row[i] + pred) & 0xff
            elif filt != 0:
                raise ValueError(f"{path}: bad PNG filter {filt}")
        for x in range(width):
            si = x * channels
            di = (y * width + x) * 4
            if channels == 4:
                out[di:di + 4] = row[si:si + 4]
            elif channels == 3:
                out[di:di + 3] = row[si:si + 3]
                out[di + 3] = 255
            else:
                out[di:di + 3] = bytes((row[si], row[si], row[si]))
                out[di + 3] = 255
        prev = row
    return width, height, out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("reference")
    ap.add_argument("candidate")
    ap.add_argument("--threshold", type=int, default=8,
                    help="per-channel difference counted as a mismatch")
    ap.add_argument("--max-mismatch", type=float, default=0.03,
                    help="allowed fraction of mismatched pixels")
    args = ap.parse_args()

    rw, rh, ref = read_png(args.reference)
    cw, ch, cand = read_png(args.candidate)
    if (rw, rh) != (cw, ch):
        print(f"FAIL size {cw}x{ch} != {rw}x{rh}")
        return 1

    total = rw * rh
    bad = 0
    channel_abs = 0
    max_diff = 0
    for i in range(0, len(ref), 4):
        diff = max(abs(ref[i + k] - cand[i + k]) for k in range(4))
        channel_abs += diff
        if diff > max_diff:
            max_diff = diff
        if diff > args.threshold:
            bad += 1

    mae = channel_abs / total
    fraction = bad / total
    print(f"{rw}x{rh} MAE={mae:.3f} max={max_diff} "
          f"mismatch>{args.threshold}={bad}/{total} ({fraction * 100:.2f}%)")
    if fraction > args.max_mismatch:
        print("FAIL")
        return 1
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
