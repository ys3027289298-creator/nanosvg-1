#!/bin/bash
# Staged verification for color-interpolation-filters (sRGB vs linearRGB).
# Results land in output/. Runs non-interactively.
set -e
cd "$(dirname "$0")/../.."
mkdir -p output/oldsrc

stage() { echo; echo "=== $1 ==="; }
pause() { sleep 3; }

stage "1/11 prepare: extract pre-change headers from git"
git show HEAD:src/nanosvg.h > output/oldsrc/nanosvg.h
git show HEAD:src/nanosvgrast.h > output/oldsrc/nanosvgrast.h
sed -e 's/int main()/int main(int argc, char** argv)/' \
    -e 's#"../example/23.svg"#argc>1?argv[1]:"../example/23.svg"#' \
    -e 's#stbi_write_png("svg.png"#stbi_write_png(argc>2?argv[2]:"svg.png"#' \
    example/example2.c > output/render.c
pause

stage "2/11 build renderer: pre-change (baseline)"
gcc -O2 -Ioutput/oldsrc -Iexample output/render.c -lm -o output/render_old
pause

stage "3/11 build renderer: current sources"
gcc -O2 -Isrc -Iexample output/render.c -lm -o output/render_new
pause

stage "4/11 render: srgb_blur.svg"
output/render_new tests/colorspace/srgb_blur.svg output/srgb_blur.png
pause

stage "5/11 render: linear_blur.svg"
output/render_new tests/colorspace/linear_blur.svg output/linear_blur.png
pause

stage "6/11 render: compare.svg"
output/render_new tests/colorspace/compare.svg output/compare.png
pause

stage "7/11 render: regression.svg"
output/render_new tests/colorspace/regression.svg output/regression.png
pause

stage "8/11 regression: render example/23.svg (baseline)"
output/render_old example/23.svg output/23_old.png
pause

stage "9/11 regression: render example/23.svg (current) + compare"
output/render_new example/23.svg output/23_new.png
if cmp -s output/23_old.png output/23_new.png; then
  echo "23.svg: identical output before/after change -> PASS"
else
  echo "23.svg: output differs -> FAIL"; exit 1
fi
pause

stage "10/11 sample blurred edges vs theoretical curves"
python3 tests/colorspace/check.py | tee output/report.txt
pause

stage "11/11 done"
echo "artifacts: output/*.png output/report.txt"
