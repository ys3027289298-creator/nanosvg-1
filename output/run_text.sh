#!/usr/bin/env bash
# Staged validation for <text> textLength / lengthAdjust support.
# Uses the stock example/example2.c rasterizer; no extra demo program.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/output"
SVG="$ROOT/tests/text"
VENV="$OUT/.venv"
PY="$VENV/bin/python"
LOG="$OUT/run.log"
mkdir -p "$OUT"
: > "$LOG"

stage() { echo; echo "==================== $1 ===================="; sleep 8; }

if [ ! -x "$PY" ]; then
    python3 -m venv "$VENV"
    "$VENV/bin/pip" -q install pillow numpy
fi

stage "阶段1/5 编译仓库自带示例 example/example2.c（含本次 text 改动）"
gcc -Wall -Wextra -I "$ROOT/src" -I "$ROOT/example" \
    -o "$OUT/example2_render" "$ROOT/example/example2.c" -lm 2>>"$LOG"
echo "build ok -> output/example2_render"

stage "阶段2/5 渲染六个测试 SVG（四个核心用例 + 两个回退用例）"
for f in "$SVG"/*.svg; do
    name="$(basename "$f" .svg)"
    (cd "$OUT" && "$OUT/example2_render" "$f" "$name.png") >>"$LOG" 2>&1
    echo "rendered $name.png"
done

stage "阶段3/5 改动前基线回归：现有 SVG 必须逐像素不变"
git -C "$ROOT" show HEAD:src/nanosvg.h > "$OUT/baseline/nanosvg.h"
gcc -I "$OUT/baseline" -o "$OUT/example2_orig" "$OUT/baseline/render_orig.c" -lm 2>>"$LOG"
(cd "$OUT" && "$OUT/example2_orig" "$ROOT/example/drawing.svg" base_drawing.png) >>"$LOG" 2>&1
(cd "$OUT" && "$OUT/example2_render" "$ROOT/example/drawing.svg" new_drawing.png) >>"$LOG" 2>&1
cmp "$OUT/base_drawing.png" "$OUT/new_drawing.png" \
    && echo "drawing.svg: byte-identical to pre-change output"

stage "阶段4/5 测量实际渲染宽度与 textLength 目标长度的偏差"
"$PY" "$SVG/measure.py" "$OUT" | tee "$OUT/measure.txt"

stage "阶段5/5 收尾"
echo "artifacts: $OUT"
ls -1 "$OUT"/*.png
echo "done."
