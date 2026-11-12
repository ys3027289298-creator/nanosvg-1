#!/usr/bin/env bash
# Staged validation for vector-effect="non-scaling-stroke".
# Renders tests/veffect/*.svg with the stock example/example2.c rasterizer.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/output"
SVG="$ROOT/tests/veffect"
LOG="$OUT/run.log"
mkdir -p "$OUT"
: > "$LOG"

stage() {
    echo
    echo "==================== $1 ===================="
    sleep 7
}

stage "阶段1/5 编译仓库现成示例 example/example2.c"
gcc -Wall -Wextra -I "$ROOT/src" -I "$ROOT/example" \
    -o "$OUT/example2_render" "$ROOT/example/example2.c" -lm 2>>"$LOG"
echo "build ok -> $OUT/example2_render"

stage "阶段2/5 渲染四个测试 SVG（输出到 output/）"
for f in "$SVG"/*.svg; do
    name="$(basename "$f" .svg)"
    (cd "$OUT" && "$OUT/example2_render" "$f" "$name.png") >>"$LOG" 2>&1
    echo "rendered $name.png"
done

stage "阶段3/5 与改动前基线做逐像素回归对比"
BASE="$OUT/baseline"
mkdir -p "$BASE"
git -C "$ROOT" show HEAD:src/nanosvg.h > "$BASE/nanosvg.h"
cp "$ROOT/src/nanosvgrast.h" "$ROOT/example/stb_image_write.h" "$BASE/"
cat > "$OUT/render_orig.c" <<'EOF'
#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
int main(int c, char** v) {
    NSVGimage* im = nsvgParseFromFile(v[1], "px", 96.0f);
    if (!im) return 2;
    int w = (int)im->width, h = (int)im->height;
    unsigned char* px = malloc(w*h*4);
    nsvgRasterize(nsvgCreateRasterizer(), im, 0, 0, 1, px, w, h, w*4);
    stbi_write_png(v[2], w, h, 4, px, w*4);
    nsvgDelete(im);
    free(px);
    return 0;
}
EOF
gcc -I "$BASE" -o "$OUT/example2_orig" "$OUT/render_orig.c" -lm 2>>"$LOG"
"$OUT/example2_orig" "$SVG/04_2x_regression_default.svg" "$OUT/04_baseline_orig.png" >>"$LOG" 2>&1
cmp "$OUT/04_2x_regression_default.png" "$OUT/04_baseline_orig.png" \
    && echo "regression sample: byte-identical to pre-change output"

stage "阶段4/5 Python 测量实测描边像素宽度与虚线间距"
python3 "$SVG/measure.py" "$OUT" | tee "$OUT/measure.txt"

stage "阶段5/5 收尾"
echo "artifacts: $OUT"
ls -1 "$OUT"/*.png
echo "done."
