//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <stdio.h>
#include <string.h>
#include <float.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

static int dumpTextSpans(const char* filename)
{
	NSVGimage* image = NULL;
	NSVGtextSpan* span;
	int n = 0;

	printf("parsing %s\n", filename);
	image = nsvgParseFromFile(filename, "px", 96.0f);
	if (image == NULL) {
		printf("Could not open SVG image.\n");
		return 1;
	}
	for (span = image->textSpans; span != NULL; span = span->next, n++) {
		printf("span[%d] \"%s\" font-size=%.4g anchor=(%.3f, %.3f) shift=%.3f baseline=(%.3f, %.3f)\n",
			n, span->text, span->fontSize,
			span->x, span->y, span->baselineShift,
			span->x, span->y - span->baselineShift);
	}
	if (n == 0)
		printf("no text spans found.\n");
	nsvgDelete(image);
	return 0;
}

int main(int argc, char** argv)
{
	NSVGimage *image = NULL;
	NSVGrasterizer *rast = NULL;
	unsigned char* img = NULL;
	int w, h;
	const char* filename = "../example/23.svg";
	const char* outname = "svg.png";

	if (argc > 1 && strcmp(argv[1], "--text") == 0) {
		if (argc < 3) {
			printf("usage: example2 --text <file.svg>\n");
			return 1;
		}
		return dumpTextSpans(argv[2]);
	}
	if (argc > 1) filename = argv[1];
	if (argc > 2) outname = argv[2];

	printf("parsing %s\n", filename);
	image = nsvgParseFromFile(filename, "px", 96.0f);
	if (image == NULL) {
		printf("Could not open SVG image.\n");
		goto error;
	}
	w = (int)image->width;
	h = (int)image->height;

	rast = nsvgCreateRasterizer();
	if (rast == NULL) {
		printf("Could not init rasterizer.\n");
		goto error;
	}

	img = malloc(w*h*4);
	if (img == NULL) {
		printf("Could not alloc image buffer.\n");
		goto error;
	}

	printf("rasterizing image %d x %d\n", w, h);
	nsvgRasterize(rast, image, 0,0,1, img, w, h, w*4);

	printf("writing %s\n", outname);
 	stbi_write_png(outname, w, h, 4, img, w*4);

error:
	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);

	return 0;
}
