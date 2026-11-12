#!/bin/sh
# Staged dash-rendering verification. No interactive input; artifacts go to output/.
set -e
cd "$(dirname "$0")/../.."
mkdir -p output build
PAUSE=3
NSTAGES=11

stage() { printf '\n===== [%s/%s] %s =====\n' "$1" "$NSTAGES" "$2"; }
pause() { sleep "$PAUSE"; }

stage 1 "Environment check"
gcc --version | head -1
python3 --version
pause

stage 2 "Build baseline renderer from pristine HEAD"
git show HEAD:example/example2.c > build/base_example2.c
gcc -O2 -o build/base_render build/base_example2.c -Isrc -Iexample -lm
pause

stage 3 "Build current renderer (repo example2)"
gcc -O2 -o build/render example/example2.c -Isrc -Iexample -lm
pause

stage 4 "Rasterize dashed line"
./build/render tests/dash/line.svg output/line.png
pause

stage 5 "Rasterize dashed circle"
./build/render tests/dash/circle.svg output/circle.png
pause

stage 6 "Rasterize dashed bezier"
./build/render tests/dash/bezier.svg output/bezier.png
pause

stage 7 "Rasterize dashed line with dashoffset"
./build/render tests/dash/offset.svg output/offset.png
pause

stage 8 "Sample strokes, measure duty cycle vs theory"
python3 tests/dash/check_dash.py | tee output/report.txt
pause

stage 9 "Pixel-identity check for elements without dash"
mkdir -p build/example build/base_run
cp example/23.svg build/example/23.svg
(cd build/base_run && ../base_render >/dev/null)
./build/render example/23.svg output/23.png >/dev/null
if cmp -s build/base_run/svg.png output/23.png; then
	echo "23.svg (no dash): baseline vs current -> IDENTICAL"
else
	echo "23.svg (no dash): MISMATCH"; exit 1
fi
pause

stage 10 "Collect artifacts"
ls -la output/
pause

stage 11 "Summary"
cat output/report.txt
echo "ALL STAGES PASSED"
