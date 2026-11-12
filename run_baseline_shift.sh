#!/bin/sh
# baseline-shift 验证：编译仓库现成 example2，跑四个用例，结果统一进 output/。
# 用法: sh run_baseline_shift.sh （全自动，无需输入）
set -e
cd "$(dirname "$0")"
OUT=output
BIN=$OUT/example2
CC=${CC:-cc}
pause() { sleep 2; }
step() { echo; echo "==== 阶段 $1: $2 ===="; }

step 1 "准备 output/"
rm -rf "$OUT"; mkdir -p "$OUT"; pause
step 2 "环境检查"
$CC --version | head -n 1; pause
step 3 "静态检查 baseline-shift 命中数"
grep -c baseline src/nanosvg.h; pause
step 4 "展示测试数据"
head -n 2 example/text-*.svg; pause
step 5 "编译 example/example2.c"
$CC -O2 -Wall -Isrc -o "$BIN" example/example2.c -lm; pause
step 6 "检查编译产物"
ls -l "$BIN"; pause
step 7 "回归渲染 example/23.svg"
"$BIN" example/23.svg "$OUT/23.png" 2>&1 | tee "$OUT/render.log"; pause
step 8 "检查渲染产物"
ls -l "$OUT/23.png"; pause
step 9 "用例一: 百分比 baseline-shift"
"$BIN" --text example/text-percent.svg | tee "$OUT/case1-percent.txt"; pause
step 10 "用例二: 长度 baseline-shift"
"$BIN" --text example/text-length.svg | tee "$OUT/case2-length.txt"; pause
step 11 "用例三: super/sub 关键字"
"$BIN" --text example/text-super.svg | tee "$OUT/case3-super.txt"; pause
step 12 "用例四: 回归(无 baseline-shift)"
"$BIN" --text example/text-plain.svg | tee "$OUT/case4-plain.txt"; pause
step 13 "校验百分比与长度偏移"
grep -q "shift=5.000" "$OUT/case1-percent.txt"
grep -q "shift=6.000" "$OUT/case2-length.txt"
grep -q "shift=-4.000" "$OUT/case2-length.txt"
echo "百分比/长度偏移 OK"; pause
step 14 "校验 super/sub 与回归"
grep -q "shift=9.900" "$OUT/case3-super.txt"
grep -q "shift=-4.000" "$OUT/case3-super.txt"
! grep -q "shift=-[0-9]\|shift=[1-9]" "$OUT/case4-plain.txt"
echo "关键字与回归 OK"; pause
step 15 "统计文本输出"
wc -l "$OUT"/case*.txt; pause
step 16 "校验渲染回归日志"
grep -q "rasterizing" "$OUT/render.log"
echo "渲染回归 OK"; pause
step 17 "结果清单"
ls -l "$OUT"
echo "全部用例通过，结果见 $OUT/"
