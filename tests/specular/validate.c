#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

static int analyze(const char* path)
{
	NSVGimage* img = nsvgParseFromFile(path, "px", 96.0f);
	if (img == NULL) { printf("%s: parse failed\n", path); return 1; }
	int w = (int)img->width, h = (int)img->height;
	NSVGrasterizer* rast = nsvgCreateRasterizer();
	unsigned char* buf = (unsigned char*)calloc((size_t)w*h*4, 1);
	nsvgRasterize(rast, img, 0, 0, 1, buf, w, h, w*4);

	int px = 0, py = 0, peak = -1, lit = 0, found = 0;
	double sx = 0, sy = 0, sw = 0, m2x = 0, m2y = 0;
	int base = 255;
	for (int i = 0; i < w*h; i++)
		if (buf[4*i+3]) base = (int)fmin(base, (buf[4*i]+buf[4*i+1]+buf[4*i+2])/3.0);
	for (int i = 0; i < w*h; i++)
		peak = (int)fmax(peak, (buf[4*i]+buf[4*i+1]+buf[4*i+2])/3.0 - base);
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			unsigned char* p = &buf[(y*w + x)*4];
			int lum = p[3] ? (p[0] + p[1] + p[2]) / 3 - base : 0;
			if (lum < 0) lum = 0;
			if (lum == peak && !found) { px = x; py = y; found = 1; }
			if (lum >= peak/2) {
				lit++;
				sx += x; sy += y; sw += 1.0;
				m2x += (double)x*x; m2y += (double)y*y;
			}
		}
	double radius = 0;
	if (sw > 0.0) {
		double vx = m2x/sw - (sx/sw)*(sx/sw);
		double vy = m2y/sw - (sy/sw)*(sy/sw);
		radius = sqrt(fabs(vx) + fabs(vy));
	}
	printf("%-12s peak=%3d at=(%2d,%2d) spot_radius=%5.2f lit_px=%d\n",
		   path, peak, px, py, radius, lit);

	free(buf);
	nsvgDeleteRasterizer(rast);
	nsvgDelete(img);
	return peak < 90 || lit < 4;
}

int main(int argc, char** argv)
{
	int failed = 0;
	for (int i = 1; i < argc; i++)
		failed += analyze(argv[i]);
	printf(failed ? "FAIL\n" : "PASS\n");
	return failed ? 1 : 0;
}
