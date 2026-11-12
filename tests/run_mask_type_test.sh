#!/usr/bin/env bash
#
# Builds and runs the mask-type verification in staged output.
# Pauses roughly two seconds between stages; the whole run takes 30-90s.
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build/mask-type-test"

pause() {
	sleep 2
}

echo "=== Stage A/C: preparing build directory ==="
mkdir -p "$BUILD_DIR"
pause
pause

echo "=== Stage B/C: compiling mask_type_test ==="
cc -std=c99 -Wall -Wextra -I"$ROOT/src" \
	"$ROOT/tests/mask_type_test.c" -lm -o "$BUILD_DIR/mask_type_test"
pause
pause

echo "=== Stage C/C: running four verification cases ==="
pause
"$BUILD_DIR/mask_type_test"
result=$?
pause
pause

if [ "$result" -eq 0 ]; then
	echo "=== Verification finished successfully ==="
else
	echo "=== Verification finished with failures (exit $result) ==="
fi
exit "$result"
