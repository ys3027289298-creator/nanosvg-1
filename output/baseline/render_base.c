#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
int main(int c, char** v) {
	NSVGimage* im = nsvgParseFromFile(v[1], "px", 96.0f);
	if (!im) return 2;
	int w = (int)im->width, h = (int)im->height;
	unsigned char* px = (unsigned char*)malloc((size_t)w*h*4);
	nsvgRasterize(nsvgCreateRasterizer(), im, 0,0,1, px, w,h, w*4);
	stbi_write_png(v[2], w,h,4,px,w*4);
	FILE* fp = fopen(v[3], "wb");
	if (fp) { fwrite(px, 1, (size_t)w*h*4, fp); fclose(fp); }
	nsvgDelete(im);
	free(px);
	return 0;
}
