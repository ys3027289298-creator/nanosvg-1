#!/usr/bin/env python3
# Pixel-exact regression: shapes without a filter must render identical
# to the baseline (git HEAD) renderer.
import sys

OUT = "output"
CASES = ["regression", "23"]

ok = True
for name in CASES:
    new = open(f"{OUT}/{name}.rgba", "rb").read()
    old = open(f"{OUT}/{name}_base.rgba", "rb").read()
    same = new == old
    ok = ok and same
    print(f"[regression] {name}: {'IDENTICAL' if same else 'DIFFERS'}")

sys.exit(0 if ok else 1)
