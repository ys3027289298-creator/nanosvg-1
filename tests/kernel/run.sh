#!/bin/bash
# Staged verification for feTurbulence kernelUnitLength / preserveAlpha support.
set -e
cd "$(dirname "$0")/../.."
mkdir -p output/ref
REF=$(mktemp -d)
cleanup() { git worktree remove --force "$REF/src-tree" 2>/dev/null; rm -rf "$REF"; }
trap cleanup EXIT

stage() { echo; echo "=== $1 ==="; sleep 3; }

stage "01/10 prepare output directory"
rm -f output/*.png output/*.rgba output/ref/*.rgba output/ref/*.png

stage "02/10 build renderer (patched sources)"
gcc -O2 -o output/render tests/kernel/render.c -Isrc -Iexample -lm

stage "03/10 build reference renderer (pristine HEAD)"
git worktree add -q "$REF/src-tree" HEAD
gcc -O2 -o output/ref/render tests/kernel/render.c -I"$REF/src-tree/src" -I"$REF/src-tree/example" -lm

stage "04/10 render kernel-unit case (custom unit, 1x)"
output/render tests/kernel/kernel-unit.svg output/kernel-unit 1

stage "05/10 render kernel-unit case (2x high-res export)"
output/render tests/kernel/kernel-unit.svg output/kernel-unit-hi 2

stage "06/10 render preserve-alpha case"
output/render tests/kernel/preserve-alpha.svg output/preserve-alpha 1

stage "07/10 render combo case (unit + preserveAlpha)"
output/render tests/kernel/combo.svg output/combo 1

stage "08/10 render regression case with both renderers"
output/render tests/kernel/regression.svg output/regression 1
output/ref/render tests/kernel/regression.svg output/ref/regression 1

stage "09/10 analyze alpha variance and grain size"
python3 tests/kernel/check.py

stage "10/10 list results"
ls -la output/
sleep 2
echo "done: results in output/"
