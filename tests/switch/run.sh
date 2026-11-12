#!/bin/bash
# <switch> 条件渲染验证：编译 example2 为 nanosvgrast，渲染四个用例并用 Python 校验。
set -e
cd "$(dirname "$0")/../.."
mkdir -p output
PAUSE=3

phase() { echo; echo "========== 阶段 $1: $2 =========="; }

phase 1/11 "清理旧产物"
rm -f output/nanosvgrast output/switch-*.png output/23.png
sleep $PAUSE

phase 2/11 "头文件语法自检"
gcc -fsyntax-only -Iexample -Isrc example/example2.c && echo "nanosvg.h 语法自检通过"
sleep $PAUSE

phase 3/11 "编译 example/nanosvgrast (example/example2.c)"
gcc -O2 -Iexample -Isrc example/example2.c -o output/nanosvgrast -lm
echo "构建完成: output/nanosvgrast"
sleep $PAUSE

phase 4/11 "冒烟渲染仓库自带 example/23.svg"
output/nanosvgrast example/23.svg output/23.png
sleep $PAUSE

phase 5/11 "渲染用例 switch-zh (NSVG_SYSTEM_LANGUAGE=zh)"
NSVG_SYSTEM_LANGUAGE=zh output/nanosvgrast tests/switch/switch-zh.svg output/switch-zh.png
sleep $PAUSE

phase 6/11 "渲染用例 switch-en (NSVG_SYSTEM_LANGUAGE=en)"
NSVG_SYSTEM_LANGUAGE=en output/nanosvgrast tests/switch/switch-en.svg output/switch-en.png
sleep $PAUSE

phase 7/11 "渲染用例 switch-fallback (NSVG_SYSTEM_LANGUAGE=zh)"
NSVG_SYSTEM_LANGUAGE=zh output/nanosvgrast tests/switch/switch-fallback.svg output/switch-fallback.png
sleep $PAUSE

phase 8/11 "渲染用例 switch-nested (NSVG_SYSTEM_LANGUAGE=zh)"
NSVG_SYSTEM_LANGUAGE=zh output/nanosvgrast tests/switch/switch-nested.svg output/switch-nested.png
sleep $PAUSE

phase 9/11 "LANG 回退对照 (LANG=zh_CN.UTF-8, 不设 NSVG_SYSTEM_LANGUAGE)"
env -u NSVG_SYSTEM_LANGUAGE LANG=zh_CN.UTF-8 output/nanosvgrast tests/switch/switch-zh.svg output/switch-zh-lang.png
cmp output/switch-zh.png output/switch-zh-lang.png && echo "LANG 回退与显式设置结果一致"
sleep $PAUSE

phase 10/11 "Python 统计非背景像素与包围盒"
python3 tests/switch/check.py
sleep $PAUSE

phase 11/11 "完成"
ls -l output/
echo "全部用例校验通过"
