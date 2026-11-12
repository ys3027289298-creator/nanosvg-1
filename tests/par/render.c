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
	if (argc < 3) {
		printf("usage: render input.svg output.png\n");
		return 1;
	}
	NSVGimage* image = nsvgParseFromFile(argv[1], "px", 96.0f);
	if (image == NULL) {
		printf("Could not open %s\n", argv[1]);
		return 1;
	}
	int w = (int)image->width, h = (int)image->height;
	NSVGrasterizer* rast = nsvgCreateRasterizer();
	unsigned char* img = malloc(w * h * 4);
	nsvgRasterize(rast, image, 0, 0, 1, img, w, h, w * 4);
	stbi_write_png(argv[2], w, h, 4, img, w * 4);
	printf("rendered %s -> %s (%d x %d)\n", argv[1], argv[2], w, h);
	free(img);
	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	return 0;
}
