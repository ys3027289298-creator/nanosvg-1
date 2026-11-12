#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

// Usage: render in.svg out.png
int main(int argc, char** argv)
{
	NSVGimage* image;
	NSVGrasterizer* rast;
	unsigned char* img;
	int w, h;

	if (argc < 3) {
		printf("usage: render in.svg out.png\n");
		return 1;
	}
	image = nsvgParseFromFile(argv[1], "px", 96.0f);
	if (image == NULL) {
		printf("could not parse %s\n", argv[1]);
		return 1;
	}
	w = (int)image->width;
	h = (int)image->height;
	img = (unsigned char*)malloc(w * h * 4);
	if (img == NULL) {
		nsvgDelete(image);
		return 1;
	}
	rast = nsvgCreateRasterizer();
	nsvgRasterize(rast, image, 0, 0, 1, img, w, h, w * 4);
	stbi_write_png(argv[2], w, h, 4, img, w * 4);
	printf("rendered %s -> %s (%d x %d)\n", argv[1], argv[2], w, h);

	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	free(img);
	return 0;
}
