#!/usr/bin/env python3
# 校验 tests/displace 各用例的位移量：对比内容矩形的实测位移与理论位移。
import os, sys

OUT = "output"
W, H = 160, 120
CONTENT = (0xFF, 0x80, 0x80)   # 内容矩形 #ff8080
BG = (0x22, 0x33, 0x44)        # 背景 #223344（无 filter，不应移动）

def load(name):
    with open(os.path.join(OUT, name + ".rgba"), "rb") as f:
        return f.read()

def bbox(px, rgb):
    xs, ys = [], []
    for y in range(H):
        for x in range(W):
            i = (y * W + x) * 4
            if tuple(px[i:i+3]) == rgb and px[i+3] == 255:
                xs.append(x); ys.append(y)
    return (min(xs), min(ys), max(xs), max(ys)) if xs else None

def shift(chan, scale):  # 通道值 -> 采样位移 -> 视觉位移（取反）
    return -(chan / 255.0 - 0.5) * scale

# 用例名, 原始矩形(x,y,w,h), 理论视觉位移(dx,dy)
CASES = [
    ("x_channel",   (50, 40, 60, 40), (shift(255, 20),  shift(128, 20))),
    ("y_channel",   (50, 40, 60, 40), (shift(128, 20),  shift(255, 20))),
    ("large_scale", (120, 40, 30, 40), (shift(255, 200), shift(128, 200))),
    ("regression",  (50, 40, 60, 40), (0.0, 0.0)),
]

print(f"{'用例':<14}{'实测位移':>14}{'理论位移':>18}  结论")
ok = True
for name, (x0, y0, w0, h0), (tdx, tdy) in CASES:
    px = load(name)
    bb = bbox(px, CONTENT)
    if bb is None:
        print(f"{name:<14} 内容矩形未找到"); ok = False; continue
    mdx = (bb[0] - x0 + bb[2] + 1 - (x0 + w0)) / 2.0
    mdy = (bb[1] - y0 + bb[3] + 1 - (y0 + h0)) / 2.0
    bg_ok = tuple(px[0:3]) == BG  # 左上角背景像素应原地不动
    good = abs(mdx - tdx) <= 1.0 and abs(mdy - tdy) <= 1.0 and bg_ok
    ok = ok and good
    print(f"{name:<14}({mdx:+6.1f},{mdy:+6.1f})   ({tdx:+7.2f},{tdy:+7.2f})  {'通过' if good else '失败'}")

print("全部通过" if ok else "存在失败用例")
sys.exit(0 if ok else 1)
