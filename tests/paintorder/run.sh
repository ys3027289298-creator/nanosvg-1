#!/bin/bash
# Staged paint-order verification. All artifacts go to output/. No input needed.
set -e
cd "$(dirname "$0")/../.."
OUT=output
START=$(date +%s)

stage() { echo; echo "===== 阶段 $1: $2 ====="; }
pause() { sleep 3; }

stage 1 "准备产物目录与基线源码"
rm -rf "$OUT"
mkdir -p "$OUT/base"
git show HEAD:src/nanosvg.h > "$OUT/base/nanosvg.h"
pause

stage 2 "确认改动范围(仅 src/nanosvg.h)"
git diff --stat
pause

stage 3 "由 example/example2.c 生成并编译当前渲染器"
sed -e 's|int main()|int main(int argc, char** argv)|' \
    -e 's|"../example/23.svg"|argv[1]|' \
    -e 's|stbi_write_png("svg.png", w, h, 4, img, w\*4);|{ FILE* f = fopen(argv[2], "wb"); fwrite(img, 1, (size_t)w*h*4, f); fclose(f); }|' \
    example/example2.c > "$OUT/nanosvgrast.c"
gcc -O2 -I src -I example "$OUT/nanosvgrast.c" -o "$OUT/nanosvgrast" -lm
pause

stage 4 "编译基线渲染器(改动前的 nanosvg.h)"
gcc -O2 -I "$OUT/base" -I src -I example "$OUT/nanosvgrast.c" -o "$OUT/nanosvgrast_base" -lm
pause

stage 5 "渲染用例 fill_first(表现属性,fill 在前)"
"$OUT/nanosvgrast" tests/paintorder/fill_first.svg "$OUT/fill_first.rgba"
pause

stage 6 "渲染用例 stroke_first(style 优先于表现属性)"
"$OUT/nanosvgrast" tests/paintorder/stroke_first.svg "$OUT/stroke_first.rgba"
pause

stage 7 "渲染用例 partial(部分取值补默认顺序)"
"$OUT/nanosvgrast" tests/paintorder/partial.svg "$OUT/partial.rgba"
pause

stage 8 "渲染回归样本(当前与基线)"
"$OUT/nanosvgrast" tests/paintorder/default.svg "$OUT/default.rgba"
"$OUT/nanosvgrast_base" tests/paintorder/default.svg "$OUT/default_base.rgba"
pause

stage 9 "回归对比:无 paint-order 输出必须逐字节一致"
if cmp "$OUT/default.rgba" "$OUT/default_base.rgba"; then
    echo "[OK] default.svg 当前与基线渲染结果逐字节一致"
else
    echo "[FAIL] default.svg 渲染结果与基线不一致"; exit 1
fi
pause

stage 10 "重叠区采样判定"
python3 tests/paintorder/check.py
pause

stage 11 "汇总产物"
ls -la "$OUT"
echo "总耗时: $(( $(date +%s) - START )) 秒"
