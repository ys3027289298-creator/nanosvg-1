#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

static int d255(int x) { return ((x + 1) * 257) >> 16; }
static int bch(int b, int s, int m) {
	if (m == 1) return d255(b * s);
	if (m == 2) return b + s - d255(b * s);
	if (m == 3) return b < 128 ? d255(2 * b * s) : 255 - d255(2 * (255 - b) * (255 - s));
	if (m == 4) return b < s ? b : s;
	if (m == 5) return b > s ? b : s;
	return s;
}

int main(int argc, char** argv) {
	(void)argc;
	NSVGimage* im = nsvgParseFromFile(argv[1], "px", 96.0f);
	if (!im) { printf("FAIL %s: parse\n", argv[1]); return 1; }
	int w = (int)im->width, h = (int)im->height;
	NSVGrasterizer* r = nsvgCreateRasterizer();
	unsigned char* px = malloc(w * h * 4);
	nsvgRasterize(r, im, 0, 0, 1, px, w, h, w * 4);
	int x = atoi(argv[2]), y = atoi(argv[3]), m = atoi(argv[4]);
	int bb[3] = {atoi(argv[5]), atoi(argv[6]), atoi(argv[7])};
	int ss[3] = {atoi(argv[8]), atoi(argv[9]), atoi(argv[10])};
	int alpha = atoi(argv[11]);
	int k = (y * w + x) * 4;
	int got[3], exp[3], pass = 1;
	for (int c = 0; c < 3; c++) {
		got[c] = px[k + c];
		exp[c] = m == 0 ? d255(ss[c] * alpha) + d255((255 - alpha) * bb[c])
		                : d255(bch(bb[c], ss[c], m) * alpha) + d255((255 - alpha) * bb[c]);
		if (abs(got[c] - exp[c]) > 2) pass = 0;
	}
	printf("%s sample(%d,%d) got rgb(%d,%d,%d) formula rgb(%d,%d,%d) %s\n",
	       argv[1], x, y, got[0], got[1], got[2], exp[0], exp[1], exp[2],
	       pass ? "PASS" : "FAIL");
	nsvgDeleteRasterizer(r);
	nsvgDelete(im);
	free(px);
	return pass ? 0 : 1;
}
