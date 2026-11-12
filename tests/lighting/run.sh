#!/usr/bin/env bash
# feDiffuseLighting end-to-end check.
# Renders four SVGs with the stock example/example2.c program, compares the
# luminance of 16 sample points per image against an independent reference
# implementation, and verifies the filter-free regression sample is unchanged.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/output"
SVG="$ROOT/tests/lighting"
BASE="$OUT/baseline"

mkdir -p "$OUT" "$BASE"

echo "==== [1/5] build the repository example (example/example2.c) ===="
gcc -std=c99 -Wall -Wextra -I "$ROOT/src" -I "$ROOT/example" \
	-o "$OUT/example2" "$ROOT/example/example2.c" -lm

echo "==== [2/5] export pre-change baseline headers from git HEAD ===="
git -C "$ROOT" show HEAD:src/nanosvg.h > "$BASE/nanosvg.h"
git -C "$ROOT" show HEAD:src/nanosvgrast.h > "$BASE/nanosvgrast.h"
cp "$ROOT/example/stb_image_write.h" "$BASE/"

cat > "$BASE/render_base.c" <<'EOF'
#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
int main(int c, char** v) {
	NSVGimage* im = nsvgParseFromFile(v[1], "px", 96.0f);
	if (!im) return 2;
	int w = (int)im->width, h = (int)im->height;
	unsigned char* px = (unsigned char*)malloc((size_t)w*h*4);
	nsvgRasterize(nsvgCreateRasterizer(), im, 0,0,1, px, w,h, w*4);
	stbi_write_png(v[2], w,h,4,px,w*4);
	FILE* fp = fopen(v[3], "wb");
	if (fp) { fwrite(px, 1, (size_t)w*h*4, fp); fclose(fp); }
	nsvgDelete(im);
	free(px);
	return 0;
}
EOF
gcc -std=c99 -I "$BASE" -o "$BASE/render_base" "$BASE/render_base.c" -lm

echo "==== [3/5] render the four test SVGs (new and baseline) ===="
for f in distant point bigbump regression; do
	(cd "$OUT" && "$OUT/example2" "$SVG/$f.svg" "$f.png" >/dev/null)
	"$BASE/render_base" "$SVG/$f.svg" "$BASE/$f.png" "$BASE/$f.rgba"
done

echo "==== [4/5] byte-identical regression for filter-free graphics ===="
for f in 23 nano drawing; do
	"$BASE/render_base" "$ROOT/example/$f.svg" "$BASE/$f.png" "$BASE/$f.rgba"
	(cd "$OUT" && "$OUT/example2" "$ROOT/example/$f.svg" "$f.png" >/dev/null)
	if cmp -s "$OUT/$f.rgba" "$BASE/$f.rgba"; then
		echo "  $f.svg: byte-identical to baseline"
	else
		echo "  $f.svg: DIFFERS FROM BASELINE" >&2
		exit 1
	fi
done

echo "==== [5/5] 16-point luminance check vs theoretical values ===="
python3 "$SVG/check.py"

echo
echo "all artifacts written to $OUT/"
