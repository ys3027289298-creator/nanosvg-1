#!/bin/bash
# feDisplacementMap 演示：编译、渲染、校验一条龙。输出统一放 output/。
set -e
cd "$(dirname "$0")/../.."
mkdir -p output

section() {
	sleep 2
	echo
	echo "========== $1 =========="
}

section "1/11 环境检查"
gcc --version | head -1
python3 --version

section "2/11 编译位移测试渲染器"
gcc -O2 -Isrc -Iexample tests/displace/render.c -o output/render -lm
echo "output/render 就绪"

section "3/11 编译仓库自带示例 example2"
gcc -O2 -Isrc -Iexample example/example2.c -o output/example2 -lm
echo "output/example2 就绪"

section "4/11 渲染用例 x_channel（X 通道位移）"
./output/render tests/displace/x_channel.svg output/x_channel
sleep 1

section "5/11 渲染用例 y_channel（Y 通道位移）"
./output/render tests/displace/y_channel.svg output/y_channel
sleep 1

section "6/11 渲染用例 large_scale（大 scale + 越界透明）"
./output/render tests/displace/large_scale.svg output/large_scale
sleep 1

section "7/11 渲染用例 regression（无 filter 回归样本）"
./output/render tests/displace/regression.svg output/regression
sleep 1

section "8/11 越界安全检查（AddressSanitizer）"
gcc -g -fsanitize=address,undefined -Isrc -Iexample tests/displace/render.c -o output/render_asan -lm
for c in x_channel y_channel large_scale regression; do
	./output/render_asan tests/displace/$c.svg output/asan_$c > /dev/null
done
echo "ASan/UBSan 无告警：越界取样按透明处理，未读越界内存"

section "9/11 仓库示例回归（example2 渲染 example/23.svg）"
(cd output && ./example2)
sleep 1

section "10/11 Python 位移校验"
python3 tests/displace/verify.py

section "11/11 输出清单"
ls -la output/
echo "演示完成，产物在 output/ 下"
