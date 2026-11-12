// Minimal SVG render harness for the kernel tests, based on example/example2.c.
// Usage: render <in.svg> <out-base> [scale]
// Writes <out-base>.png (inspection) and <out-base>.rgba (raw w,h + RGBA for check.py).
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
	char path[512];
	FILE* fp;
	float scale;
	int w, h;

	if (argc < 3) {
		printf("usage: render <in.svg> <out-base> [scale]\n");
		return 1;
	}
	scale = argc > 3 ? (float)atof(argv[3]) : 1.0f;

	image = nsvgParseFromFile(argv[1], "px", 96.0f);
	if (image == NULL) { printf("could not parse %s\n", argv[1]); return 1; }
	w = (int)(image->width * scale);
	h = (int)(image->height * scale);
	img = (unsigned char*)malloc(w * h * 4);
	rast = nsvgCreateRasterizer();
	if (img == NULL || rast == NULL) { printf("out of memory\n"); return 1; }

	nsvgRasterize(rast, image, 0, 0, scale, img, w, h, w * 4);

	snprintf(path, sizeof(path), "%s.png", argv[2]);
	stbi_write_png(path, w, h, 4, img, w * 4);
	snprintf(path, sizeof(path), "%s.rgba", argv[2]);
	fp = fopen(path, "wb");
	fwrite(&w, sizeof(int), 1, fp);
	fwrite(&h, sizeof(int), 1, fp);
	fwrite(img, 4, w * h, fp);
	fclose(fp);

	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	free(img);
	return 0;
}
