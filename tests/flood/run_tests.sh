#!/bin/sh
# Build the small renderer, render the four samples, run the pixel checks.
set -e
cd "$(dirname "$0")/../.."
mkdir -p output
gcc -O2 tests/flood/render.c -o output/render -lm
for n in flood_solid offset10 flood_offset_chain regression; do
	output/render tests/flood/$n.svg output/$n.rgba output/$n.png
done
python3 tests/flood/verify.py output
