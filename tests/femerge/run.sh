#!/bin/sh
# feMerge 多输入合并验证：构建 -> 四个用例 -> 重叠区采样与公式比对。
# 所有产物写入 output/，无需人工输入。
set -e
cd "$(dirname "$0")/../.."
mkdir -p output
stage() { echo; echo "===== $1 ====="; sleep 2; }

stage "1/6 构建仓库现成 example（example2 原样 + argv 派生版）"
cc -O2 -I src -I example example/example2.c -o output/ex2 -lm
sed -e 's/int main()/int main(int argc, char** argv)/' \
    -e 's|"../example/23.svg"|argv[1]|' \
    -e 's|stbi_write_png("svg.png"|stbi_write_png(argv[2]|' \
    example/example2.c > output/render.c
cc -O2 -I src -I example output/render.c -o output/render -lm
sleep 2

stage "2/6 用例一：两层合并 merge2.svg"
./output/render tests/femerge/merge2.svg output/merge2.png
sleep 2

stage "3/6 用例二：四层合并（feMerge result 被后续引用）merge4.svg"
./output/render tests/femerge/merge4.svg output/merge4.png
sleep 2

stage "4/6 用例三：引用不存在的 result 按透明处理 merge_missing.svg"
./output/render tests/femerge/merge_missing.svg output/merge_missing.png
sleep 2

stage "5/6 回归：未改动 example 渲染 example/23.svg"
(cd output && ./ex2)
sleep 2

stage "6/6 重叠区采样与 source-over 公式比对"
python3 tests/femerge/check.py

stage "7/7 结果归档 output/"
ls -1 output/
echo
echo "全部完成，产物见 output/"
