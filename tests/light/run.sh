#!/bin/sh
# fePointLight regression: the specular highlight must track the light position.
# For each case, render the SVG, find the brightest pixel and compare it
# against the expected light position (deviation in pixels).
set -e
cd "$(dirname "$0")"
cc -O2 -I../../src -o render_light render_light.c -lm
trap 'rm -f render_light' EXIT

check() { # name svg expect_x expect_y tolerance
	got=$(./render_light "$2")
	gx=${got% *}
	gy=${got#* }
	dev=$(awk -v x="$gx" -v y="$gy" -v ex="$3" -v ey="$4" \
		'BEGIN { printf "%.2f", sqrt((x-ex)^2 + (y-ey)^2) }')
	echo "$1: measured=($gx,$gy) expected=($3,$4) deviation=${dev}px"
	awk -v d="$dev" -v t="$5" 'BEGIN { exit (d+0 <= t+0) ? 0 : 1 }' \
		|| { echo "$1: FAIL (tolerance $5)"; exit 1; }
}

check top-left     tl.svg       40.5  30.5 1.0
check bottom-right br.svg      160.5 120.5 1.0
check low-z        z.svg       100.5  75.5 1.0
check regression   regress.svg 120    40   1.0
echo "all fePointLight cases passed"
