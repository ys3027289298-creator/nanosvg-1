// Renders an SVG to PNG (+ raw RGBA) and prints parsed filter parameters.
// Usage: render <input.svg> <output-prefix>
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
	NSVGfilter* f;
	NSVGshape* s;
	unsigned char* img;
	int w, h;
	char path[512];
	FILE* fp;

	if (argc < 3) { printf("usage: render <in.svg> <out-prefix>\n"); return 1; }
	image = nsvgParseFromFile(argv[1], "px", 96.0f);
	if (image == NULL) { printf("could not parse %s\n", argv[1]); return 1; }

	for (f = image->filters; f != NULL; f = f->next)
		printf("filter #%s: in=%s in2=%s scale=%g xChannel=%c yChannel=%c flood=#%06x\n",
			f->id, f->in[0] ? f->in : "SourceGraphic", f->in2[0] ? f->in2 : "SourceGraphic",
			f->scale, "RGBA"[f->xChannelSelector & 3], "RGBA"[f->yChannelSelector & 3],
			f->hasFlood ? ((f->floodColor & 0xff) << 16 | (f->floodColor & 0xff00) | ((f->floodColor >> 16) & 0xff)) : 0);
	for (s = image->shapes; s != NULL; s = s->next)
		if (s->filter[0]) printf("shape filter -> #%s\n", s->filter);

	w = (int)image->width; h = (int)image->height;
	img = (unsigned char*)malloc(w * h * 4);
	rast = nsvgCreateRasterizer();
	nsvgRasterize(rast, image, 0, 0, 1, img, w, h, w * 4);

	snprintf(path, sizeof(path), "%s.png", argv[2]);
	stbi_write_png(path, w, h, 4, img, w * 4);
	snprintf(path, sizeof(path), "%s.rgba", argv[2]);
	fp = fopen(path, "wb");
	if (fp) { fwrite(img, 1, (size_t)w * h * 4, fp); fclose(fp); }
	printf("rendered %s -> %s.png (%dx%d)\n", argv[1], argv[2], w, h);

	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	free(img);
	return 0;
}
