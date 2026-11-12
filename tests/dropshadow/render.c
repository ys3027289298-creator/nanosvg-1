// Renders an SVG to <out>.png and raw RGBA to <out>.rgba for inspection.
// Usage: render input.svg out_prefix
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
	NSVGimage* image = NULL;
	NSVGrasterizer* rast = NULL;
	unsigned char* img = NULL;
	char path[1024];
	FILE* fp;
	int w, h;

	if (argc < 3) {
		printf("usage: %s input.svg out_prefix\n", argv[0]);
		return 1;
	}
	image = nsvgParseFromFile(argv[1], "px", 96.0f);
	if (image == NULL) { printf("could not parse %s\n", argv[1]); return 1; }
	w = (int)image->width;
	h = (int)image->height;
	rast = nsvgCreateRasterizer();
	img = (unsigned char*)malloc(w*h*4);
	if (rast == NULL || img == NULL) { printf("out of memory\n"); return 1; }

	nsvgRasterize(rast, image, 0,0,1, img, w, h, w*4);

	snprintf(path, sizeof(path), "%s.png", argv[2]);
	stbi_write_png(path, w, h, 4, img, w*4);
	snprintf(path, sizeof(path), "%s.rgba", argv[2]);
	fp = fopen(path, "wb");
	if (fp) { fwrite(img, 1, (size_t)w*h*4, fp); fclose(fp); }
	printf("rendered %s -> %s.{png,rgba} (%dx%d)\n", argv[1], argv[2], w, h);

	free(img);
	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	return 0;
}
