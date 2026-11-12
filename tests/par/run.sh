#!/usr/bin/env bash
# BUG-4412: preserveAspectRatio validation, end-to-end via the bundled rasterizer.
set -e
cd "$(dirname "$0")/../.."
OUT=output
hdr() { printf '\n===== %s =====\n' "$1"; sleep 5; }

hdr "stage 1/5: build renderer with fixed nanosvg"
gcc -O2 -Wall -Isrc -Iexample tests/par/render.c -lm -o "$OUT/render_fixed"
sleep 3

hdr "stage 2/5: render four par samples into output/"
for f in tests/par/*.svg; do
  "$OUT/render_fixed" "$f" "$OUT/$(basename "${f%.svg}").png"
done
sleep 2

hdr "stage 3/5: rebuild baseline (pre-fix header from git) for regression"
mkdir -p "$OUT/baseline/inc"
git show HEAD:src/nanosvg.h    > "$OUT/baseline/inc/nanosvg.h"
git show HEAD:src/nanosvgrast.h > "$OUT/baseline/inc/nanosvgrast.h"
cp example/stb_image_write.h "$OUT/baseline/inc/"
gcc -O2 -I"$OUT/baseline/inc" tests/par/render.c -lm -o "$OUT/baseline/render"
"$OUT/baseline/render" tests/par/par_default.svg "$OUT/baseline/par_default.png"
sleep 2

hdr "stage 4/5: bbox and deviation check (python, stdlib only)"
python3 tests/par/check.py "$OUT"
sleep 2

hdr "stage 5/5: render bundled example2 as sanity check"
gcc -O2 -Isrc -Iexample example/example2.c -lm -o "$OUT/example2"
(cd "$OUT" && ./example2)
printf '\n===== done; artifacts in output/ =====\n'
