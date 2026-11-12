#!/usr/bin/env bash
# Staged verification for mix-blend-mode + isolation support in NanoSVG.
set -u
cd "$(dirname "$0")/.."
mkdir -p output
LOG=output/blend_results.txt
: > "$LOG"

stage() { echo; echo "== $1 ==" | tee -a "$LOG"; sleep 6; }

stage "Stage 1/5: build existing example2 and blend harness"
cc -O2 -I src example/example2.c -o output/example2 -lm
cc -O2 -I src example/blend_check.c -o output/blend_check -lm

stage "Stage 2/5: run repository example (regression smoke test)"
(cd example && ../output/example2) && mv example/svg.png output/example2.png && echo "example2 ok" | tee -a "$LOG"

stage "Stage 3/5: multiply and overlay blend cases"
output/blend_check example/blend_multiply.svg 6 6 1 128 128 128 255 64 64 255 | tee -a "$LOG"
output/blend_check example/blend_overlay.svg 6 6 3 32 160 224 160 96 32 255 | tee -a "$LOG"
for m in screen darken lighten; do
	printf '<svg xmlns="http://www.w3.org/2000/svg" width="12" height="12"><rect width="12" height="12" fill="#2060a0"/><rect x="4" y="4" width="8" height="8" fill="#4080c0" style="mix-blend-mode:%s"/></svg>\n' "$m" > output/be_$m.svg
done
output/blend_check output/be_screen.svg  6 6 2 32 96 160 64 128 192 255 | tee -a "$LOG"
output/blend_check output/be_darken.svg  6 6 4 32 96 160 64 128 192 255 | tee -a "$LOG"
output/blend_check output/be_lighten.svg 6 6 5 32 96 160 64 128 192 255 | tee -a "$LOG"

stage "Stage 4/5: isolation before/after and normal-alpha regression"
output/blend_check example/blend_isolate_off.svg 6 6 1 79 79 79 255 64 64 255 | tee -a "$LOG"
output/blend_check example/blend_isolate_on.svg 6 6 1 126 126 126 255 64 64 255 | tee -a "$LOG"
output/blend_check example/blend_regression.svg 6 6 0 128 128 128 255 64 64 128 | tee -a "$LOG"

stage "Stage 5/5: summary"
sleep 6
if grep -q FAIL "$LOG"; then echo "RESULT: FAIL" | tee -a "$LOG"; exit 1; fi
echo "RESULT: all cases PASS (artifacts in output/)" | tee -a "$LOG"
