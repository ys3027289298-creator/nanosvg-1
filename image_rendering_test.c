// Test for 'image-rendering' attribute support (auto / pixelated / optimizeSpeed).
//
// Cases:
//   1. upscale 4x with image-rendering="pixelated" -> nearest-neighbor sampling
//   2. downscale with image-rendering="pixelated"  -> nearest-neighbor sampling
//   3. image-rendering="auto" (and "optimizeSpeed") -> same as default rendering
//   4. no attribute -> output identical to default rendering (regression)
//
// For each case the ratio of identical adjacent pixels is printed, and the
// program judges whether nearest-neighbor sampling was used by comparing
// against an exact 1:1 render.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NANOSVG_IMPLEMENTATION
#include "src/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "src/nanosvgrast.h"

static char* loadFile(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	char* data;
	long size;
	if (f == NULL) return NULL;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = (char*)malloc((size_t)size + 1);
	if (data == NULL) { fclose(f); return NULL; }
	if (fread(data, 1, (size_t)size, f) != (size_t)size) { fclose(f); free(data); return NULL; }
	data[size] = '\0';
	fclose(f);
	return data;
}

// Returns a copy of the svg with image-rendering="<value>" injected into the
// root <svg> tag, or an unmodified copy if value is NULL.
static char* injectImageRendering(const char* svg, const char* value)
{
	char* copy;
	char* tag;
	char* end;
	char* out;
	size_t head, tail, attrLen;
	char attr[128];

	if (value == NULL) {
		copy = (char*)malloc(strlen(svg) + 1);
		if (copy) strcpy(copy, svg);
		return copy;
	}
	snprintf(attr, sizeof(attr), " image-rendering=\"%s\"", value);
	tag = strstr(svg, "<svg");
	if (tag == NULL) return NULL;
	end = strchr(tag, '>');
	if (end == NULL) return NULL;
	head = (size_t)(end - svg);
	tail = strlen(end);
	attrLen = strlen(attr);
	out = (char*)malloc(head + attrLen + tail + 1);
	if (out == NULL) return NULL;
	memcpy(out, svg, head);
	memcpy(out + head, attr, attrLen);
	memcpy(out + head + attrLen, end, tail + 1);
	return out;
}

static NSVGimage* parseVariant(const char* svg, const char* rendering)
{
	char* text = injectImageRendering(svg, rendering);
	NSVGimage* image;
	if (text == NULL) return NULL;
	image = nsvgParse(text, "px", 96.0f);
	free(text);
	return image;
}

static unsigned char* render(NSVGrasterizer* rast, NSVGimage* image, float scale, int* outW, int* outH)
{
	int w = (int)(image->width * scale);
	int h = (int)(image->height * scale);
	unsigned char* img;
	if (w < 1) w = 1;
	if (h < 1) h = 1;
	img = (unsigned char*)malloc((size_t)w * (size_t)h * 4);
	if (img == NULL) return NULL;
	nsvgRasterize(rast, image, 0, 0, scale, img, w, h, w * 4);
	*outW = w;
	*outH = h;
	return img;
}

// Ratio of adjacent pixels (right and down neighbors) that have identical RGBA values.
static double adjacentIdenticalRatio(const unsigned char* img, int w, int h)
{
	long same = 0, total = 0;
	int x, y;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			const unsigned char* p = &img[((size_t)y * w + x) * 4];
			if (x + 1 < w) {
				const unsigned char* q = p + 4;
				total++;
				if (memcmp(p, q, 4) == 0) same++;
			}
			if (y + 1 < h) {
				const unsigned char* q = &img[((size_t)(y + 1) * w + x) * 4];
				total++;
				if (memcmp(p, q, 4) == 0) same++;
			}
		}
	}
	return total > 0 ? (double)same / (double)total : 0.0;
}

// Exact nearest-neighbor check: every destination pixel must equal the pixel
// of the 1:1 render it was sampled from.
// Pixels sampling outside the reference render (fractional edge band) are
// skipped and reported in *skipped.
static long nearestNeighborMismatch(const unsigned char* img, int w, int h,
									const unsigned char* ref, int refw, int refh, float scale,
									long* skipped)
{
	long mismatch = 0;
	int x, y;
	*skipped = 0;
	for (y = 0; y < h; y++) {
		int sy = (int)floorf(y / scale);
		for (x = 0; x < w; x++) {
			int sx = (int)floorf(x / scale);
			if (sx < 0 || sx >= refw || sy < 0 || sy >= refh) {
				(*skipped)++;
				continue;
			}
			if (memcmp(&img[((size_t)y * w + x) * 4], &ref[((size_t)sy * refw + sx) * 4], 4) != 0)
				mismatch++;
		}
	}
	return mismatch;
}

int main(int argc, char** argv)
{
	const char* filename = argc > 1 ? argv[1] : "example/23.svg";
	char* svg;
	NSVGimage *imgPixelated, *imgAuto, *imgSpeed, *imgDefault;
	NSVGrasterizer* rast;
	unsigned char *up4, *down, *auto4, *speed4, *def4, *def1, *pix1, *ref1;
	int w, h, rw, rh, tmpW, tmpH;
	double ratio;
	long nnMismatch, nnSkipped;
	int failed = 0;

	svg = loadFile(filename);
	if (svg == NULL) { printf("FAIL: cannot read %s\n", filename); return 1; }

	imgPixelated = parseVariant(svg, "pixelated");
	imgAuto = parseVariant(svg, "auto");
	imgSpeed = parseVariant(svg, "optimizeSpeed");
	imgDefault = parseVariant(svg, NULL);
	if (!imgPixelated || !imgAuto || !imgSpeed || !imgDefault) {
		printf("FAIL: cannot parse %s\n", filename);
		return 1;
	}
	if (imgPixelated->imageRendering != NSVG_IMAGE_RENDERING_PIXELATED ||
		imgAuto->imageRendering != NSVG_IMAGE_RENDERING_AUTO ||
		imgSpeed->imageRendering != NSVG_IMAGE_RENDERING_OPTIMIZE_SPEED ||
		imgDefault->imageRendering != NSVG_IMAGE_RENDERING_AUTO) {
		printf("FAIL: image-rendering attribute not parsed as expected\n");
		return 1;
	}

	rast = nsvgCreateRasterizer();
	if (rast == NULL) { printf("FAIL: cannot create rasterizer\n"); return 1; }

	// 1:1 reference render of the default image for nearest-neighbor checks.
	ref1 = render(rast, imgDefault, 1.0f, &rw, &rh);

	// Case 1: upscale 4x, pixelated.
	up4 = render(rast, imgPixelated, 4.0f, &w, &h);
	ratio = adjacentIdenticalRatio(up4, w, h);
	nnMismatch = nearestNeighborMismatch(up4, w, h, ref1, rw, rh, 4.0f, &nnSkipped);
	printf("case1 upscale 4x pixelated: adjacent-identical ratio = %.4f, nn mismatches = %ld (edge skipped = %ld)\n",
		ratio, nnMismatch, nnSkipped);
	if (nnMismatch == 0 && ratio > 0.5) {
		printf("  -> nearest-neighbor sampling: YES (PASS)\n");
	} else {
		printf("  -> nearest-neighbor sampling: NO (FAIL)\n");
		failed = 1;
	}

	// Case 2: downscale 0.25x, pixelated.
	down = render(rast, imgPixelated, 0.25f, &w, &h);
	ratio = adjacentIdenticalRatio(down, w, h);
	nnMismatch = nearestNeighborMismatch(down, w, h, ref1, rw, rh, 0.25f, &nnSkipped);
	printf("case2 downscale 0.25x pixelated: adjacent-identical ratio = %.4f, nn mismatches = %ld (edge skipped = %ld)\n",
		ratio, nnMismatch, nnSkipped);
	if (nnMismatch == 0) {
		printf("  -> nearest-neighbor sampling: YES (PASS)\n");
	} else {
		printf("  -> nearest-neighbor sampling: NO (FAIL)\n");
		failed = 1;
	}

	// Case 3: upscale 4x, auto (and optimizeSpeed) must match default rendering.
	auto4 = render(rast, imgAuto, 4.0f, &w, &h);
	speed4 = render(rast, imgSpeed, 4.0f, &tmpW, &tmpH);
	def4 = render(rast, imgDefault, 4.0f, &tmpW, &tmpH);
	ratio = adjacentIdenticalRatio(auto4, w, h);
	nnMismatch = nearestNeighborMismatch(auto4, w, h, ref1, rw, rh, 4.0f, &nnSkipped);
	printf("case3 upscale 4x auto: adjacent-identical ratio = %.4f, nn mismatches = %ld\n", ratio, nnMismatch);
	if (memcmp(auto4, def4, (size_t)w * h * 4) == 0 &&
		memcmp(speed4, def4, (size_t)w * h * 4) == 0 &&
		nnMismatch != 0) {
		printf("  -> nearest-neighbor sampling: NO, auto == optimizeSpeed == default (PASS)\n");
	} else {
		printf("  -> auto/optimizeSpeed differ from default or used nearest-neighbor (FAIL)\n");
		failed = 1;
	}

	// Case 4: regression, no attribute. Default path must be untouched:
	// scale 1 renders identically with and without the attribute logic, and
	// pixelated at scale 1 falls back to the default path.
	def1 = render(rast, imgDefault, 1.0f, &w, &h);
	pix1 = render(rast, imgPixelated, 1.0f, &tmpW, &tmpH);
	ratio = adjacentIdenticalRatio(def1, w, h);
	printf("case4 no attribute scale 1: adjacent-identical ratio = %.4f\n", ratio);
	if (memcmp(def1, ref1, (size_t)w * h * 4) == 0 &&
		memcmp(pix1, def1, (size_t)w * h * 4) == 0) {
		printf("  -> default rendering unchanged (PASS)\n");
	} else {
		printf("  -> default rendering changed (FAIL)\n");
		failed = 1;
	}

	free(up4); free(down); free(auto4); free(speed4);
	free(def4); free(def1); free(pix1); free(ref1);
	nsvgDeleteRasterizer(rast);
	nsvgDelete(imgPixelated);
	nsvgDelete(imgAuto);
	nsvgDelete(imgSpeed);
	nsvgDelete(imgDefault);
	free(svg);

	printf(failed ? "RESULT: FAIL\n" : "RESULT: PASS\n");
	return failed;
}
