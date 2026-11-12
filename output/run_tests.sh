#!/bin/bash
# Verify glyph-orientation support using the repo's own example (example2).
# Prints one titled section per case, pauses between sections (~40s total).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/output"
BIN="$OUT/example2"
PAUSE=8

echo "== build: example2 (repo example) =="
cc -O2 -I"$ROOT/src" -I"$ROOT/example" "$ROOT/example/example2.c" -o "$BIN" -lm
sleep 2

run_case() { # $1 title, $2 input svg, $3 output png
  echo
  echo "== $1 =="
  (cd "$OUT" && "$BIN" "$2" "$3")
  sleep "$PAUSE"
}

run_case "case 1/4: glyph-orientation-horizontal=0 + transform" \
  "$OUT/text-orient-0.svg" "$OUT/text-orient-0.png"

run_case "case 2/4: glyph-orientation-vertical=90 (vertical-rl)" \
  "$OUT/text-orient-90.svg" "$OUT/text-orient-90.png"

run_case "case 3/4: glyph-orientation=auto along textPath tangent" \
  "$OUT/text-orient-auto.svg" "$OUT/text-orient-auto.png"

run_case "case 4/4: regression, repo example/23.svg (no text)" \
  "$ROOT/example/23.svg" "$OUT/regression-23.png"

echo
echo "== done: artifacts in output/ =="
