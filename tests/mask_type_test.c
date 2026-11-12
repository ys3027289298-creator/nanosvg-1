//
// Verification for SVG mask support: mask-type luminance vs alpha.
//
// Four cases are rendered off-screen and checked by sampling the alpha of
// a pixel fully inside the masked region:
//   1. Grayscale mask, default (luminance) mask-type
//   2. Grayscale mask, explicit alpha mask-type
//   3. Colored (red) mask, luminance mask-type
//   4. Regression sample without any mask
//

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#define IMG_SIZE 24
#define TOLERANCE 2

static void stage_pause(void)
{
	struct timespec ts;
	ts.tv_sec = 2;
	ts.tv_nsec = 0;
	nanosleep(&ts, NULL);
}

static void stage_title(const char* title)
{
	stage_pause();
	printf("\n=== %s ===\n", title);
	fflush(stdout);
	stage_pause();
}

// Renders an SVG string and returns the alpha channel at the center pixel,
// which is always fully covered by both the mask and the masked shape.
static int sample_center_alpha(const char* svg_text)
{
	char svg[2048];
	NSVGimage* image;
	NSVGrasterizer* rast;
	unsigned char* buffer;
	int alpha;

	strncpy(svg, svg_text, sizeof(svg) - 1);
	svg[sizeof(svg) - 1] = '\0';

	image = nsvgParse(svg, "px", 96.0f);
	if (image == NULL) {
		printf("  ERROR: failed to parse SVG\n");
		return -1;
	}

	rast = nsvgCreateRasterizer();
	if (rast == NULL) {
		nsvgDelete(image);
		printf("  ERROR: failed to create rasterizer\n");
		return -1;
	}

	buffer = (unsigned char*)calloc((size_t)IMG_SIZE * IMG_SIZE * 4, 1);
	nsvgRasterize(rast, image, 0, 0, 1, buffer, IMG_SIZE, IMG_SIZE, IMG_SIZE * 4);

	alpha = buffer[((IMG_SIZE / 2) * IMG_SIZE + IMG_SIZE / 2) * 4 + 3];

	free(buffer);
	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	return alpha;
}

static int check(const char* name, const char* svg, int theory)
{
	int measured = sample_center_alpha(svg);
	int ok = measured >= 0 && abs(measured - theory) <= TOLERANCE;

	printf("  case            : %s\n", name);
	printf("  measured alpha  : %d\n", measured);
	printf("  theoretical alpha: %d\n", theory);
	printf("  result          : %s (tolerance +/-%d)\n",
	       ok ? "PASS" : "FAIL", TOLERANCE);
	return ok ? 0 : 1;
}

int main(void)
{
	int failures = 0;

	// Case 1: grayscale mask, mask-type omitted -> luminance per spec.
	// #808080 has luma 128, full opacity, so the coefficient is 128/255.
	const char* grayscale_luminance =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\">"
		"<defs><mask id=\"m\">"
		"<rect x=\"0\" y=\"0\" width=\"24\" height=\"24\" fill=\"#808080\"/>"
		"</mask></defs>"
		"<rect x=\"0\" y=\"0\" width=\"24\" height=\"24\" fill=\"#ffffff\" mask=\"url(#m)\"/>"
		"</svg>";

	// Case 2: same grayscale rect, but mask-type alpha. The mask alpha is
	// 255 regardless of color, so the white rect keeps full alpha.
	const char* grayscale_alpha =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\">"
		"<defs><mask id=\"m\" mask-type=\"alpha\">"
		"<rect x=\"0\" y=\"0\" width=\"24\" height=\"24\" fill=\"#808080\"/>"
		"</mask></defs>"
		"<rect x=\"0\" y=\"0\" width=\"24\" height=\"24\" fill=\"#ffffff\" mask=\"url(#m)\"/>"
		"</svg>";

	// Case 3: colored mask with default luminance type.
	// Red (255, 0, 0) luma = 0.2126 * 255 ~= 54.
	const char* colored_luminance =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\">"
		"<defs><mask id=\"m\">"
		"<rect x=\"0\" y=\"0\" width=\"24\" height=\"24\" fill=\"#ff0000\"/>"
	"</mask></defs>"
		"<rect x=\"0\" y=\"0\" width=\"24\" height=\"24\" fill=\"#ffffff\" mask=\"url(#m)\"/>"
		"</svg>";

	// Case 4: no mask at all; output must be unchanged (fully opaque rect).
	const char* no_mask_regression =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\">"
		"<rect x=\"0\" y=\"0\" width=\"24\" height=\"24\" fill=\"#00ff00\"/>"
		"</svg>";

	printf("NanoSVG mask-type verification");
	printf("\nSampling alpha at masked region center (%d,%d)", IMG_SIZE / 2, IMG_SIZE / 2);

	stage_title("Stage 1/4: grayscale mask, default mask-type (luminance)");
	failures += check("grayscale / luminance (default)", grayscale_luminance, 128);

	stage_title("Stage 2/4: grayscale mask, mask-type=alpha");
	failures += check("grayscale / alpha", grayscale_alpha, 255);

	stage_title("Stage 3/4: colored mask, luminance weighted RGB");
	failures += check("colored red / luminance", colored_luminance, 54);

	stage_title("Stage 4/4: regression sample, no mask");
	failures += check("no mask / unchanged output", no_mask_regression, 255);

	stage_pause();
	printf("\n=== Summary ===\n");
	if (failures == 0)
		printf("All 4 cases PASS\n");
	else
		printf("%d case(s) FAIL\n", failures);

	return failures == 0 ? 0 : 1;
}
