#!/bin/sh
# Phased verification for gradientTransform / spreadMethod support.
# No keyboard input required; artifacts go to output/.
cd "$(dirname "$0")/.."
FAIL=0

echo "===== [1/11] Prepare output dirs and pristine baseline sources ====="
mkdir -p output/baseline-src
git show HEAD:src/nanosvg.h > output/baseline-src/nanosvg.h
git show HEAD:src/nanosvgrast.h > output/baseline-src/nanosvgrast.h
sleep 3

echo "===== [2/11] Generate rasterizer from example/example2.c ====="
sed -e 's/^int main()/int main(int argc, char** argv)/' \
    -e 's|"../example/23.svg"|(argc > 1 ? argv[1] : "../example/23.svg")|' \
    -e 's|stbi_write_png("svg.png", w, h, 4, img, w\*4);|{ const char* out = (argc > 2 ? argv[2] : "svg.raw"); FILE* f = fopen(out, "wb"); fwrite(img, 1, (size_t)w*h*4, f); fclose(f); }|' \
    example/example2.c > output/nanosvgrast.c
echo "generated output/nanosvgrast.c"
sleep 3

echo "===== [3/11] Compile current rasterizer ====="
gcc -O2 -o output/nanosvgrast output/nanosvgrast.c -Isrc -Iexample -lm || exit 1
echo "compiled output/nanosvgrast"
sleep 3

echo "===== [4/11] Compile baseline (pre-change) rasterizer ====="
gcc -O2 -o output/nanosvgrast-baseline output/nanosvgrast.c -Ioutput/baseline-src -Iexample -lm || exit 1
echo "compiled output/nanosvgrast-baseline"
sleep 3

echo "===== [5/11] Render rotated-linear.svg (gradientTransform rotate) ====="
./output/nanosvgrast tests/gradient/rotated-linear.svg output/rotated-linear.raw
./output/nanosvgrast-baseline tests/gradient/rotated-linear.svg output/before-rotated-linear.raw
sleep 3

echo "===== [6/11] Render reflected-radial.svg (spreadMethod=reflect) ====="
./output/nanosvgrast tests/gradient/reflected-radial.svg output/reflected-radial.raw
sleep 3

echo "===== [7/11] Render repeated-linear.svg + regression-default.svg ====="
./output/nanosvgrast tests/gradient/repeated-linear.svg output/repeated-linear.raw
./output/nanosvgrast tests/gradient/regression-default.svg output/regression-default.raw
./output/nanosvgrast-baseline tests/gradient/regression-default.svg output/baseline-regression.raw
sleep 3

echo "===== [8/11] Sample pixels along gradient direction vs theory ====="
python3 output/check_gradient.py || FAIL=1
sleep 3

echo "===== [9/11] Before/after comparison (rotated-linear, y=30) ====="
python3 -c "
b=open('output/before-rotated-linear.raw','rb').read()
a=open('output/rotated-linear.raw','rb').read()
for x in (10,30,50,70,90,110):
    i=(30*120+x)*4
    print('  x=%3d before=(%3d,%3d,%3d) after=(%3d,%3d,%3d)'%(x,b[i],b[i+1],b[i+2],a[i],a[i+1],a[i+2]))
"
sleep 3

echo "===== [10/11] Regression: default gradient must be pixel-identical ====="
if cmp -s output/baseline-regression.raw output/regression-default.raw; then
    echo "regression-default: PASS (pixel-identical to pre-change render)"
else
    echo "regression-default: FAIL (output differs)"; FAIL=1
fi
sleep 3

echo "===== [11/11] Summary ====="
ls -la output/
sleep 3
echo "===== Done (overall: $([ $FAIL -eq 0 ] && echo PASS || echo FAIL)) ====="
exit $FAIL
