# Clip path regression fixtures

- `01_shape.svg`: one shape clipped by a circle.
- `02_group.svg`: a group clip shared by two children.
- `03_nested.svg`: nested group clips, intersected.
- `04_object_bbox.svg`: `objectBoundingBox` clip units.
- `05_evenodd.svg`: a rectangular ring using `clip-rule="evenodd"`.
- `06_missing_ref.svg`: missing `clip-path` reference, rendered unclipped.

Run all fixtures against librsvg:

```bash
tests/clips/run_tests.sh
```

If `rsvg-convert` is not installed, set `RSVG_CONVERT` to its path. The
comparison allows at most 3% of pixels to differ by more than 8/255 per
channel, which covers normal antialiasing differences between rasterizers.
