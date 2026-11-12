#!/usr/bin/env bash
# feDropShadow end-to-end check: build, render, analyze, regression diff.
# Every section prints a title, sections are separated by a 2s pause.
set -e
cd "$(dirname "$0")/../.."
OUT=output
mkdir -p "$OUT/baseline"

step() { echo; echo "===== $1 ====="; sleep 2; }

step "[01/16] 检查测试素材"
ls -1 tests/dropshadow/

step "[02/16] 编译当前渲染器（含 feDropShadow）"
gcc -O2 -Isrc -Iexample tests/dropshadow/render.c -o "$OUT/render" -lm

step "[03/16] 导出基线头文件（git HEAD，改动前）"
git show HEAD:src/nanosvg.h > "$OUT/baseline/nanosvg.h"
git show HEAD:src/nanosvgrast.h > "$OUT/baseline/nanosvgrast.h"

step "[04/16] 编译基线渲染器"
gcc -O2 -I"$OUT/baseline" -Iexample tests/dropshadow/render.c -o "$OUT/render_base" -lm

step "[05/16] 渲染用例 offset-positive（正向偏移硬阴影）"
"$OUT/render" tests/dropshadow/offset-positive.svg "$OUT/offset-positive"

step "[06/16] 渲染用例 offset-negative（负向偏移）"
"$OUT/render" tests/dropshadow/offset-negative.svg "$OUT/offset-negative"

step "[07/16] 渲染用例 big-blur（大模糊）"
"$OUT/render" tests/dropshadow/big-blur.svg "$OUT/big-blur"

step "[08/16] 渲染用例 regression（无 filter 回归样本）"
"$OUT/render" tests/dropshadow/regression.svg "$OUT/regression"

step "[09/16] 渲染 example/23.svg（当前渲染器）"
"$OUT/render" example/23.svg "$OUT/23"

step "[10/16] 基线渲染 regression.svg"
"$OUT/render_base" tests/dropshadow/regression.svg "$OUT/regression_base"

step "[11/16] 基线渲染 example/23.svg"
"$OUT/render_base" example/23.svg "$OUT/23_base"

step "[12/16] 编译仓库现成示例 example2"
gcc -O2 -Isrc -Iexample example/example2.c -o "$OUT/example2" -lm

step "[13/16] 运行 example2（渲染 23.svg）"
(cd "$OUT" && ./example2)

step "[14/16] Python 分析阴影包围盒与平均 alpha"
python3 tests/dropshadow/analyze.py

step "[15/16] 回归逐像素对比（新渲染器 vs 基线）"
python3 tests/dropshadow/regression.py

step "[16/16] 汇总"
ls -1 "$OUT"/*.png
echo "全部产物已写入 $OUT/"
