// Renders an SVG and prints the centroid of the brightest pixels as "x y"
// (in SVG user coordinates, i.e. pixel centers are at integer+0.5).
#include <stdio.h>
#include <stdlib.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

int main(int argc, char** argv)
{
	NSVGimage* img;
	NSVGrasterizer* rast;
	unsigned char* rgba;
	int w, h, x, y;
	unsigned int best = 0;
	double sx = 0.0, sy = 0.0, n = 0.0;

	if (argc < 2) return 2;
	img = nsvgParseFromFile(argv[1], "px", 96);
	if (img == NULL) { fprintf(stderr, "cannot parse %s\n", argv[1]); return 2; }
	w = (int)img->width;
	h = (int)img->height;
	rgba = (unsigned char*)malloc((size_t)w * (size_t)h * 4);
	rast = nsvgCreateRasterizer();
	if (rgba == NULL || rast == NULL) return 2;

	nsvgRasterize(rast, img, 0.0f, 0.0f, 1.0f, rgba, w, h, w * 4);
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			unsigned char* px = &rgba[(y * w + x) * 4];
			unsigned int lum = (unsigned int)px[0] + px[1] + px[2];
			if (px[3] > 0 && lum > best) best = lum;
		}
	}
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			unsigned char* px = &rgba[(y * w + x) * 4];
			if (px[3] > 0 && (unsigned int)px[0] + px[1] + px[2] == best) {
				sx += x + 0.5; sy += y + 0.5; n += 1.0;
			}
		}
	}
	printf("%.2f %.2f\n", n > 0.0 ? sx / n : -1.0, n > 0.0 ? sy / n : -1.0);

	nsvgDeleteRasterizer(rast);
	nsvgDelete(img);
	free(rgba);
	return 0;
}
