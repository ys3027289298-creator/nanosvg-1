#include <stdio.h>
#include <string.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

static int midtones(const char* svg, int w, int h)
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s", svg);
	NSVGimage* img = nsvgParse(buf, "px", 96.0f);
	NSVGrasterizer* rast = nsvgCreateRasterizer();
	unsigned char* px = (unsigned char*)calloc(w * h, 4);
	nsvgRasterize(rast, img, 0, 0, 1, px, w, h, w * 4);
	int n = 0;
	for (int i = 0; i < w * h; i++)
		if (px[4 * i + 3] > 0 && px[4 * i + 3] < 255) n++;
	free(px);
	nsvgDeleteRasterizer(rast);
	nsvgDelete(img);
	return n;
}

int main(void)
{
	int fail = 0;
	const char* head = "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24'>";
	const char* circle = "<circle cx='12' cy='12' r='8' fill='black'";
	struct { const char* name, *svg; int want; } cases[] = {
		{ "crispEdges circle",
		  "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24'>"
		  "<circle cx='12' cy='12' r='8' fill='black' shape-rendering='crispEdges'/></svg>", 0 },
		{ "auto circle",
		  "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24'>"
		  "<circle cx='12' cy='12' r='8' fill='black' shape-rendering='auto'/></svg>", 1 },
		{ "crispEdges diagonal",
		  "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24'>"
		  "<polygon points='2,22 22,2 22,5 5,22' fill='black' shape-rendering='crispEdges'/></svg>", 0 },
	};
	for (int i = 0; i < 3; i++) {
		int m = midtones(cases[i].svg, 24, 24);
		int ok = cases[i].want ? m > 0 : m == 0;
		printf("%-22s midtones=%d %s\n", cases[i].name, m, ok ? "ok" : "FAIL");
		if (!ok) fail = 1;
	}
	const char* modes[] = { "auto", "optimizeSpeed", "geometricPrecision", "" };
	for (int k = 0; k < 4; k++) {
		char svg[256], attr[64] = "";
		if (modes[k][0]) snprintf(attr, sizeof(attr), " shape-rendering='%s'", modes[k]);
		snprintf(svg, sizeof(svg), "%s%s%s/></svg>", head, circle, attr);
		int m = midtones(svg, 24, 24);
		int ok = m > 0;
		printf("regression %-18s midtones=%d %s\n", modes[k][0] ? modes[k] : "(none)", m, ok ? "ok" : "FAIL");
		if (!ok) fail = 1;
	}
	printf(fail ? "FAIL\n" : "ALL PASS\n");
	return fail;
}
