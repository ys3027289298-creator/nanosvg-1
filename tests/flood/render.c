#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../example/stb_image_write.h"
#define NANOSVG_IMPLEMENTATION
#include "../../src/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../../src/nanosvgrast.h"

int main(int argc, char** argv)
{
	if (argc < 4) {
		printf("usage: %s in.svg out.rgba out.png\n", argv[0]);
		return 1;
	}
	NSVGimage* image = nsvgParseFromFile(argv[1], "px", 96.0f);
	if (image == NULL) { printf("parse failed: %s\n", argv[1]); return 1; }
	int w = (int)image->width, h = (int)image->height;
	NSVGrasterizer* rast = nsvgCreateRasterizer();
	unsigned char* img = malloc((size_t)w * h * 4);
	nsvgRasterize(rast, image, 0, 0, 1, img, w, h, w * 4);
	FILE* fp = fopen(argv[2], "wb");
	fwrite(img, 1, (size_t)w * h * 4, fp);
	fclose(fp);
	stbi_write_png(argv[3], w, h, 4, img, w * 4);
	free(img);
	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	return 0;
}
