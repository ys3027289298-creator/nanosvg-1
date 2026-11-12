// Rasterize one SVG file to a PNG. Test entry point for clip-path support.
#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

int main(int argc, char** argv)
{
	NSVGimage* image;
	NSVGrasterizer* rast;
	unsigned char* img;
	int w, h;

	if (argc < 3) {
		printf("usage: %s input.svg output.png\n", argv[0]);
		return 1;
	}

	image = nsvgParseFromFile(argv[1], "px", 96.0f);
	if (image == NULL) {
		printf("Could not parse %s\n", argv[1]);
		return 1;
	}
	w = (int)image->width;
	h = (int)image->height;

	rast = nsvgCreateRasterizer();
	if (rast == NULL) { nsvgDelete(image); return 1; }

	img = (unsigned char*)malloc((size_t)w*h*4);
	if (img == NULL) { nsvgDeleteRasterizer(rast); nsvgDelete(image); return 1; }

	nsvgRasterize(rast, image, 0, 0, 1, img, w, h, w*4);
	stbi_write_png(argv[2], w, h, 4, img, w*4);

	free(img);
	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	return 0;
}
