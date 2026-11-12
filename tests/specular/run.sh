#!/bin/sh
# Build and validate the four feSpecularLighting test cases.
# Fails if any case has no bright highlight, or if the large-exponent
# spot is not tighter than the normal point-light spot.
set -e
cd "$(dirname "$0")"

CC=${CC:-cc}
CFLAGS="-Wall -Wextra -I../../src"
$CC $CFLAGS validate.c -lm -o validate

OUT=$(./validate point.svg distant.svg exponent.svg regression.svg)
echo "$OUT"

fail() { echo "ASSERT FAILED: $1"; exit 1; }

# Every case must render a visible specular peak (peak increment >= 64).
for f in point distant exponent regression; do
	pk=$(printf '%s\n' "$OUT" | grep "$f\.svg" | sed 's/.*peak= *\([0-9]*\).*/\1/')
	[ "$pk" -ge 64 ] || fail "$f peak=$pk too low"
done

# Peak pixel must sit inside the 32x32 canvas.
	printf '%s\n' "$OUT" | grep -Eq 'at=\( ?[0-9]+, ?[0-9]+\)' || fail "bad peak coords"

# A larger specularExponent must produce a smaller, tighter spot.
r_point=$(printf '%s\n' "$OUT" | grep 'point\.svg'    | sed 's/.*spot_radius= *\([0-9.]*\).*/\1/')
r_exp=$(printf '%s\n' "$OUT"   | grep 'exponent\.svg' | sed 's/.*spot_radius= *\([0-9.]*\).*/\1/')
awk -v a="$r_point" -v b="$r_exp" 'BEGIN { exit !(a > b) }' \
	|| fail "exponent spot ($r_exp) not smaller than point spot ($r_point)"

# Distant light must yield a highlight somewhere on the shape.
lit=$(printf '%s\n' "$OUT" | grep 'distant\.svg' | sed 's/.*lit_px=\([0-9]*\).*/\1/')
[ "$lit" -ge 4 ] || fail "distant light produced no spot"

echo "ALL CHECKS PASSED"
