#include <stdio.h>
#include <stdlib.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

int main(int argc, char** argv)
{
	NSVGimage* image = nsvgParseFromFile(argv[1], "px", 96.0f);
	if (!image) { fprintf(stderr, "parse fail: %s\n", argv[1]); return 1; }
	int w = (int)image->width, h = (int)image->height;
	NSVGrasterizer* rast = nsvgCreateRasterizer();
	unsigned char* buf = malloc((size_t)w*h*4);
	nsvgRasterize(rast, image, 0, 0, 1, buf, w, h, w*4);
	fwrite(&w, sizeof(int), 1, stdout);
	fwrite(&h, sizeof(int), 1, stdout);
	fwrite(buf, (size_t)w*h*4, 1, stdout);
	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	free(buf);
	return 0;
}
