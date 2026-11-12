#!/bin/sh
# feMerge / feComposite regression demo: titled sections, 2s pauses, no input.
cd "$(dirname "$0")/../.." || exit 1
OUT=output
FAIL=0
START=$(date +%s)

section() { echo; echo "== $1 =="; }

section "1/16 环境检查"
cc --version | head -1
python3 --version
sleep 2

section "2/16 准备输出目录"
mkdir -p $OUT && echo "ok: $OUT/"
sleep 2

section "3/16 构建渲染工具"
cc -O2 -I src -I example tests/composite/render.c -o $OUT/render -lm && echo "ok: $OUT/render"
sleep 2

section "4/16 构建仓库现成示例 example2"
cc -O2 -I src -I example example/example2.c -o $OUT/example2 -lm && echo "ok: $OUT/example2"
sleep 2

section "5/16 运行仓库现成示例 (无 filter 回归)"
(cd $OUT && ./example2)
sleep 2

section "6/16 渲染回归样例 drawing.svg"
$OUT/render example/drawing.svg $OUT/drawing.png
sleep 2

section "7/16 渲染回归样例 nano.svg"
$OUT/render example/nano.svg $OUT/nano.png
sleep 2

section "8/16 渲染用例 merge_two_layers"
$OUT/render tests/composite/merge_two_layers.svg $OUT/merge_two_layers.png
sleep 2

section "9/16 渲染用例 composite_over"
$OUT/render tests/composite/composite_over.svg $OUT/composite_over.png
sleep 2

section "10/16 渲染用例 composite_in"
$OUT/render tests/composite/composite_in.svg $OUT/composite_in.png
sleep 2

section "11/16 渲染用例 composite_xor"
$OUT/render tests/composite/composite_xor.svg $OUT/composite_xor.png
sleep 2

section "12/16 校验用例 merge_two_layers"
python3 tests/composite/check_pixels.py merge_two_layers || FAIL=1
sleep 2

section "13/16 校验用例 composite_over"
python3 tests/composite/check_pixels.py composite_over || FAIL=1
sleep 2

section "14/16 校验用例 composite_in"
python3 tests/composite/check_pixels.py composite_in || FAIL=1
sleep 2

section "15/16 校验用例 composite_xor"
python3 tests/composite/check_pixels.py composite_xor || FAIL=1
sleep 2

section "16/16 汇总"
echo "产物:" && ls -1 $OUT
echo "耗时: $(( $(date +%s) - START ))s"
[ $FAIL -eq 0 ] && echo "结果: 全部通过" || echo "结果: 存在失败用例"
exit $FAIL
