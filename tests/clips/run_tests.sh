#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)
cd "$root"

gcc -std=c99 -O2 -I src example/example2.c -lm -o build/example2
mkdir -p tests/clips/out/nanosvg tests/clips/out/rsvg

rsvg_convert=${RSVG_CONVERT:-rsvg-convert}
if ! command -v "$rsvg_convert" >/dev/null 2>&1; then
	echo "rsvg-convert not found; set RSVG_CONVERT to a local binary" >&2
	exit 2
fi

fail=0
for svg in tests/clips/[0-9][0-9]_*.svg; do
	name=$(basename "$svg" .svg)
	./build/example2 "$svg" "tests/clips/out/nanosvg/$name.png" >/dev/null
	"$rsvg_convert" -w 200 -h 160 -o "tests/clips/out/rsvg/$name.png" "$svg"
	echo "== $name =="
	python3 tests/clips/compare_png.py \
		"tests/clips/out/rsvg/$name.png" \
		"tests/clips/out/nanosvg/$name.png" || fail=1
done

exit "$fail"
