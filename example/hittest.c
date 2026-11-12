#include <stdio.h>
#include <string.h>
#define NANOSVG_IMPLEMENTATION
#include "../src/nanosvg.h"

static const char* svg =
	"<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'>"
	"<rect id='fill-stroke' x='10' y='10' width='30' height='30' fill='#f00' stroke='#000' stroke-width='4'/>"
	"<rect id='stroke-only' x='60' y='10' width='30' height='30' fill='none' stroke='#000' stroke-width='4'/>"
	"<rect id='below' x='10' y='60' width='30' height='30' fill='#00f'/>"
	"<rect id='ghost' x='15' y='65' width='30' height='30' fill='#0f0' pointer-events='none'/>"
	"<rect id='under' x='60' y='60' width='30' height='30' fill='#00f'/>"
	"<rect id='over' x='70' y='70' width='20' height='20' fill='#ff0' fill-opacity='0' pointer-events='visibleFill'/>"
	"<rect id='tp' x='45' y='60' width='10' height='30' fill='#000' fill-opacity='0'/>"
	"</svg>";

typedef struct { float x, y; const char* want; } Case;

int main(void)
{
	static const Case cases[] = {
		{25, 25, "fill-stroke"}, // fill+stroke: interior
		{11, 25, "fill-stroke"}, // fill+stroke: on the stroke
		{75, 25, "-"},           // stroke only: interior not hit
		{61, 25, "stroke-only"}, // stroke only: on the stroke
		{20, 70, "below"},       // none: ghost skipped, shape below hit
		{42, 92, "-"},           // none: area covered by ghost only
		{65, 65, "under"},       // overlap: only under
		{75, 75, "over"},        // overlap: topmost wins (visibleFill, transparent fill)
		{50, 70, "-"},           // transparent fill + default visiblePainted: no hit
	};
	char buf[1024];
	NSVGimage* img;
	int i, fails = 0;
	strcpy(buf, svg); // nsvgParse modifies the input string
	img = nsvgParse(buf, "px", 96);
	if (img == NULL) { printf("parse failed\n"); return 1; }
	for (i = 0; i < (int)(sizeof(cases)/sizeof(cases[0])); i++) {
		NSVGshape* s = nsvgHitTest(img, cases[i].x, cases[i].y);
		const char* got = s ? s->id : "-";
		int ok = strcmp(got, cases[i].want) == 0;
		if (!ok) fails++;
		printf("(%5.1f,%5.1f) hit=%-12s expect=%-12s %s\n",
			cases[i].x, cases[i].y, got, cases[i].want, ok ? "OK" : "FAIL");
	}
	nsvgDelete(img);
	if (fails) { printf("FAILED %d\n", fails); return 1; }
	printf("ALL OK\n");
	return 0;
}
