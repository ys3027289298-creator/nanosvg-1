/*
 * enable-background / filter coverage test.
 *
 * Four cases exercise the filter pipeline added to nanosvg.h:
 *   1. new        - filter input is the shape alone, freshly captured background
 *   2. accumulate - previously drawn content is merged into the filter input
 *                   and the processed region grows to cover it
 *   3. combo      - blur + color-matrix brightness, accumulate background
 *   4. regression - no enable-background / no filter, output stays unchanged
 *
 * For every case the covered (alpha > 16) bounding box, covered pixel count
 * and average straight RGBA over the covered pixels are printed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define CANVAS_W 200
#define CANVAS_H 200
#define ALPHA_THRESHOLD 16

typedef struct Stats {
	int x0, y0, x1, y1;	// coverage bounding box, x1/y1 inclusive; empty => x1 < x0
	int count;			// covered pixel count
	double avg[4];		// average straight RGBA over covered pixels
} Stats;

typedef struct RenderCtx {
	NSVGrasterizer* rast;
} RenderCtx;

// Renders a single shape isolated into a zeroed buffer. Used both by the
// filter pipeline (layer rasterization) and the test orchestration.
static void renderShape(NSVGshape* shape, void* user,
						unsigned char* buf, int w, int h, int stride)
{
	NSVGshape* next = shape->next;
	unsigned char flags = shape->flags;
	NSVGimage image;

	memset(buf, 0, (size_t)stride * h);

	shape->next = NULL;
	memset(&image, 0, sizeof(image));
	image.width = (float)w;
	image.height = (float)h;
	image.shapes = shape;

	nsvgRasterize(((RenderCtx*)user)->rast, &image, 0, 0, 1, buf, w, h, stride);

	shape->next = next;
	shape->flags = flags;
}

static void computeStats(const unsigned char* img, int w, int h, int stride, Stats* st)
{
	int x, y;
	double sum[4] = {0, 0, 0, 0};

	st->x0 = w; st->y0 = h; st->x1 = -1; st->y1 = -1;
	st->count = 0;

	for (y = 0; y < h; y++) {
		const unsigned char* row = img + y * stride;
		for (x = 0; x < w; x++) {
			const unsigned char* px = &row[x * 4];
			if (px[3] <= ALPHA_THRESHOLD)
				continue;
			if (st->count == 0) {
				st->x0 = st->x1 = x;
				st->y0 = st->y1 = y;
			} else {
				if (x < st->x0) st->x0 = x;
				if (x > st->x1) st->x1 = x;
				if (y < st->y0) st->y0 = y;
				if (y > st->y1) st->y1 = y;
			}
			for (int c = 0; c < 4; c++)
				sum[c] += px[c];
			st->count++;
		}
	}

	for (int c = 0; c < 4; c++)
		st->avg[c] = st->count ? sum[c] / st->count : 0.0;
}

static void printStats(const char* tag, const Stats* st)
{
	if (st->count == 0) {
		printf("  %-10s coverage: empty\n", tag);
		return;
	}
	printf("  %-10s bbox=[%d,%d-%d,%d] %dx%d area=%d avgRGBA=(%.2f, %.2f, %.2f, %.2f)\n",
		   tag, st->x0, st->y0, st->x1, st->y1,
		   st->x1 - st->x0 + 1, st->y1 - st->y0 + 1, st->count,
		   st->avg[0], st->avg[1], st->avg[2], st->avg[3]);
}

static NSVGshape* findShape(NSVGimage* image, const char* id)
{
	NSVGshape* shape;
	for (shape = image->shapes; shape != NULL; shape = shape->next) {
		if (strcmp(shape->id, id) == 0)
			return shape;
	}
	return NULL;
}

// Renders everything before "filteredId", then routes the named shape through
// the filter pipeline. Remaining shapes are drawn normally.
static void renderScene(NSVGimage* image, RenderCtx* ctx,
						const char* filteredId, unsigned char* dst)
{
	NSVGshape* shape;
	int w = CANVAS_W, h = CANVAS_H, stride = CANVAS_W * 4;

	memset(dst, 0, (size_t)stride * h);

	for (shape = image->shapes; shape != NULL; shape = shape->next) {
		if (filteredId && strcmp(shape->id, filteredId) == 0) {
			nsvgApplyShapeFilter(shape, dst, w, h, stride, renderShape, ctx);
		} else {
			NSVGimage single;
			NSVGshape* next = shape->next;
			unsigned char flags = shape->flags;

			shape->next = NULL;
			memset(&single, 0, sizeof(single));
			single.width = (float)w;
			single.height = (float)h;
			single.shapes = shape;
			nsvgRasterize(ctx->rast, &single, 0, 0, 1, dst, w, h, stride);
			shape->next = next;
			shape->flags = flags;
		}
	}
}

// Red backdrop occupying a broad band, plus a blue circle in the middle
// carrying the filter. The blur halo is where missing background shows.
static const char* sceneSvg(const char* rootAttr, const char* shapeAttr)
{
	static char buf[2048];
	snprintf(buf, sizeof(buf),
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" %s>"
		"<rect id=\"backdrop\" x=\"10\" y=\"70\" width=\"180\" height=\"60\" fill=\"#d03030\"/>"
		"<circle id=\"target\" cx=\"100\" cy=\"100\" r=\"26\" fill=\"#2060e0\" %s/>"
		"</svg>",
		CANVAS_W, CANVAS_H, rootAttr, shapeAttr);
	return buf;
}

static int runCase(const char* name, const char* rootAttr, const char* shapeAttr,
				   RenderCtx* ctx, Stats* out)
{
	char* data;
	NSVGimage* image;
	NSVGshape* target;
	unsigned char* dst;
	char path[256];
	size_t len;

	const char* svg = sceneSvg(rootAttr, shapeAttr);
	len = strlen(svg) + 1;
	data = (char*)malloc(len);
	memcpy(data, svg, len);

	image = nsvgParse(data, "px", 96.0f);
	if (image == NULL) {
		printf("[%s] failed to parse SVG\n", name);
		free(data);
		return 0;
	}

	target = findShape(image, "target");
	if (target == NULL) {
		printf("[%s] target shape missing\n", name);
		nsvgDelete(image);
		free(data);
		return 0;
	}

	dst = (unsigned char*)malloc((size_t)CANVAS_W * CANVAS_H * 4);
	renderScene(image, ctx, "target", dst);
	computeStats(dst, CANVAS_W, CANVAS_H, CANVAS_W * 4, out);

	snprintf(path, sizeof(path), "%s.png", name);
	stbi_write_png(path, CANVAS_W, CANVAS_H, 4, dst, CANVAS_W * 4);

	printf("[%s] filterKind=%d enableBackground=%d blur=%.2f brightness=%.2f -> %s\n",
		   name, target->filterKind, target->enableBackground,
		   target->filterBlur, target->filterBrightness, path);

	free(dst);
	nsvgDelete(image);
	free(data);
	return 1;
}

// Regression: identical SVG rendered with and without the filter pipeline
// must produce byte-identical buffers when no filter is attached.
static int runRegression(RenderCtx* ctx)
{
	const char* svg =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"200\">"
		"<rect x=\"10\" y=\"70\" width=\"180\" height=\"60\" fill=\"#d03030\"/>"
		"<circle cx=\"100\" cy=\"100\" r=\"26\" fill=\"#2060e0\"/>"
		"</svg>";
	char* dataA;
	char* dataB;
	NSVGimage *imageA, *imageB;
	unsigned char *dstA, *dstB;
	Stats stats;
	size_t len = strlen(svg) + 1;
	int identical;

	dataA = (char*)malloc(len);
	dataB = (char*)malloc(len);
	memcpy(dataA, svg, len);
	memcpy(dataB, svg, len);

	imageA = nsvgParse(dataA, "px", 96.0f);
	imageB = nsvgParse(dataB, "px", 96.0f);
	if (imageA == NULL || imageB == NULL) {
		printf("[regression] parse failed\n");
		return 0;
	}

	dstA = (unsigned char*)malloc((size_t)CANVAS_W * CANVAS_H * 4);
	dstB = (unsigned char*)malloc((size_t)CANVAS_W * CANVAS_H * 4);

	renderScene(imageA, ctx, NULL, dstA);
	renderScene(imageB, ctx, NULL, dstB);

	identical = memcmp(dstA, dstB, (size_t)CANVAS_W * CANVAS_H * 4) == 0;
	computeStats(dstA, CANVAS_W, CANVAS_H, CANVAS_W * 4, &stats);

	printf("[regression] no filter, no enable-background, pipeline bypassed\n");
	printStats("output", &stats);
	printf("  %-10s byte-identical to plain nsvgRasterize: %s\n",
		   "check", identical ? "PASS" : "FAIL");

	stbi_write_png("regression.png", CANVAS_W, CANVAS_H, 4, dstA, CANVAS_W * 4);

	free(dstA);
	free(dstB);
	nsvgDelete(imageA);
	nsvgDelete(imageB);
	free(dataA);
	free(dataB);
	return identical;
}

// Default enable-background: a filtered shape with no attribute must keep
// the current canvas outside its own (blurred) bounds untouched.
static int runDefaultFilter(RenderCtx* ctx, Stats* out)
{
	const char* svg =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"200\">"
		"<rect id=\"backdrop\" x=\"10\" y=\"70\" width=\"180\" height=\"60\" fill=\"#d03030\"/>"
		"<circle id=\"target\" cx=\"100\" cy=\"100\" r=\"26\" fill=\"#2060e0\" filter=\"blur(8)\"/>"
		"</svg>";
	size_t len = strlen(svg) + 1;
	char* data = (char*)malloc(len);
	NSVGimage* image;
	NSVGshape* target;
	unsigned char* dst;

	memcpy(data, svg, len);
	image = nsvgParse(data, "px", 96.0f);
	if (image == NULL)
		return 0;
	target = findShape(image, "target");
	if (target == NULL || target->enableBackground != NSVG_ENABLE_BG_DEFAULT)
		return 0;

	dst = (unsigned char*)malloc((size_t)CANVAS_W * CANVAS_H * 4);
	renderScene(image, ctx, "target", dst);
	computeStats(dst, CANVAS_W, CANVAS_H, CANVAS_W * 4, out);

	// The backdrop pixels beyond the blurred shape halo must remain untouched.
	int untouched = 1;
	int x, y;
	for (y = 0; y < CANVAS_H; y++) {
		for (x = 0; x < CANVAS_W; x++) {
			unsigned char* px = &dst[(y * CANVAS_W + x) * 4];
			if (x < out->x0 || x > out->x1 || y < out->y0 || y > out->y1) {
				if (x >= 10 && x < 190 && y >= 70 && y < 130 && px[3] != 255)
					untouched = 0;
			}
		}
	}

	printf("[default] filter without enable-background, backdrop outside halo preserved\n");
	printStats("output", out);
	printf("  %-10s backdrop outside halo untouched: %s\n",
		   "check", untouched ? "PASS" : "FAIL");
	stbi_write_png("default.png", CANVAS_W, CANVAS_H, 4, dst, CANVAS_W * 4);

	free(dst);
	nsvgDelete(image);
	free(data);
	return untouched;
}

int main(void)
{
	RenderCtx ctx;
	Stats stNew, stAcc, stCombo, stDefault;
	int pass = 1;

	ctx.rast = nsvgCreateRasterizer();
	if (ctx.rast == NULL) {
		printf("could not create rasterizer\n");
		return 1;
	}

	// Case 1: enable-background="new" on the filtered shape.
	pass &= runCase("new", "", "enable-background=\"new\" filter=\"blur(8)\"",
					&ctx, &stNew);

	// Case 2: enable-background="accumulate" inherited from the root <svg>.
	pass &= runCase("accumulate", "enable-background=\"accumulate\"",
					"filter=\"blur(8)\"", &ctx, &stAcc);

	// Case 3: combo filter (blur + brightness) with accumulate.
	pass &= runCase("combo", "enable-background=\"accumulate\"",
					"filter=\"blur(6) brightness(1.6)\"", &ctx, &stCombo);

	printStats("new", &stNew);
	printStats("accumulate", &stAcc);
	printStats("combo", &stCombo);

	// Case 4: regression.
	pass &= runRegression(&ctx);

	// Default (no attribute) filter path.
	pass &= runDefaultFilter(&ctx, &stDefault);

	// Direct comparisons requested by the design.
	printf("\ncomparison:\n");
	printf("  accumulate vs new covered width:  %d vs %d (%s)\n",
		   stAcc.x1 - stAcc.x0 + 1, stNew.x1 - stNew.x0 + 1,
		   (stAcc.x1 - stAcc.x0) > (stNew.x1 - stNew.x0) ? "expanded" : "same");
	printf("  accumulate vs new covered height: %d vs %d (%s)\n",
		   stAcc.y1 - stAcc.y0 + 1, stNew.y1 - stNew.y0 + 1,
		   (stAcc.y1 - stAcc.y0) > (stNew.y1 - stNew.y0) ? "expanded" : "same");
	printf("  accumulate vs new covered area:   %d vs %d\n",
		   stAcc.count, stNew.count);
	printf("  combo vs accumulate avg red:      %.2f vs %.2f (brightness lifts it)\n",
		   stCombo.avg[0], stAcc.avg[0]);

	nsvgDeleteRasterizer(ctx.rast);
	return pass ? 0 : 1;
}
