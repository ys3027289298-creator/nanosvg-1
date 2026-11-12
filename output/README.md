# feSpotLight verification artifacts

Everything in this directory is generated. To reproduce (from the repo root,
no interactive input required):

```sh
cc -O2 -Wall -o output/render_spot output/render_spot.c -lm
cd output && ./render_spot && python3 measure.py
```

## Files

- `render_spot.c` - renders a flat 400x400 surface lit by `feSpotLight`
  through `feDiffuseLighting` and `feSpecularLighting` (implementation in
  `../src/nanosvglight.h`), one PNG per case+primitive, plus the SVGs.
- `measure.py` - dependency-free PNG decoder + measurement. Prints measured
  vs theoretical lit-region radius (cone/plane intersection) and the radial
  falloff for the narrow-cone case. Exits non-zero on failure.
- `spot_front.*` - light straight above the surface, aiming straight down
  (cone 30 deg): circular lit disc, r = z*tan(cone).
- `spot_oblique.*` - light aimed off-axis at (260,200,0) (cone 20 deg):
  elliptical lit region, verifies pointsAtX/Y/Z direction handling.
- `spot_narrow.*` - small cone (10 deg) with specularExponent=4: verifies
  the cone edge falloff against the closed form (N.L)*(-L.S)^exp.
- `spot_regression.*` - no limitingConeAngle: whole plane must be lit
  (coverage 100%), guarding the default/no-cone path.
