#!/bin/sh
# Build and run the shape-rendering test cases.
set -e

cd "$(dirname "$0")/.."

CC=${CC:-cc}
CFLAGS=${CFLAGS:-"-std=gnu11 -Wall -Wextra -O2"}
BUILD_DIR=build/tests

mkdir -p "$BUILD_DIR"
$CC $CFLAGS -Isrc tests/shape_rendering.c -lm -o "$BUILD_DIR/shape_rendering"
"$BUILD_DIR/shape_rendering"
