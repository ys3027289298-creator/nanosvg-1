/*
 * render_spot.c - verification renderer for feSpotLight (nanosvglight.h)
 *
 * Renders a flat 400x400 surface (z = 0, normal = +z) lit by an
 * feSpotLight through feDiffuseLighting and feSpecularLighting, for
 * four cases: front-on, oblique aim, narrow cone, and a regression
 * case without limitingConeAngle. Writes one PNG per case+primitive
 * and the matching .svg files. All artifacts land in output/.
 *
 * Build (from repo root):
 *   cc -O2 -o output/render_spot output/render_spot.c -lm
 * Run (from output/):
 *   ./render_spot
 */

#include <stdio.h>
#include <math.h>

#define NANOSVGLIGHT_IMPLEMENTATION
#include "../src/nanosvglight.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../example/stb_image_write.h"

#define W 400
#define H 400

typedef struct SpotCase {
	const char* name;
	NSVGspotLight light;
	int hasCone;          /* 0 = omit limitingConeAngle (regression) */
} SpotCase;

static const float WHITE[3] = { 1.0f, 1.0f, 1.0f };

static void render(const SpotCase* sc, int specular, const char* path)
{
	unsigned char img[W * H * 3];
	int px, py, c;

	for (py = 0; py < H; py++) {
		for (px = 0; px < W; px++) {
			float qx = (float)px + 0.5f;
			float qy = (float)py + 0.5f;
			float rgb[3];
			if (specular)
				nsvgSpecularSpot(&sc->light, qx, qy, 0.0f, 0, 0, 1, 1.0f, 1.0f, WHITE, rgb);
			else
				nsvgDiffuseSpot(&sc->light, qx, qy, 0.0f, 0, 0, 1, 1.0f, WHITE, rgb);
			for (c = 0; c < 3; c++) {
				float v = rgb[c] < 0 ? 0 : (rgb[c] > 1 ? 1 : rgb[c]);
				img[(py * W + px) * 3 + c] = (unsigned char)(v * 255.0f + 0.5f);
			}
		}
	}
	stbi_write_png(path, W, H, 3, img, W * 3);
}

static void writeSvg(const SpotCase* sc, const char* path)
{
	FILE* f = fopen(path, "w");
	const NSVGspotLight* l = &sc->light;
	if (!f) return;
	fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n", W, H, W, H);
	fprintf(f, "  <filter id=\"spot\" filterUnits=\"userSpaceOnUse\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\">\n", W, H);
	fprintf(f, "    <feDiffuseLighting in=\"SourceGraphic\" lighting-color=\"white\" diffuseConstant=\"1\" surfaceScale=\"1\" result=\"d\">\n");
	fprintf(f, "      <feSpotLight x=\"%g\" y=\"%g\" z=\"%g\" pointsAtX=\"%g\" pointsAtY=\"%g\" pointsAtZ=\"%g\"",
		l->x, l->y, l->z, l->pointsAtX, l->pointsAtY, l->pointsAtZ);
	if (sc->hasCone)
		fprintf(f, " limitingConeAngle=\"%g\"", l->limitingConeAngle);
	fprintf(f, " specularExponent=\"%g\"/>\n", l->specularExponent);
	fprintf(f, "    </feDiffuseLighting>\n");
	fprintf(f, "    <feSpecularLighting in=\"SourceGraphic\" lighting-color=\"white\" specularConstant=\"1\" specularExponent=\"1\" surfaceScale=\"1\" result=\"s\">\n");
	fprintf(f, "      <feSpotLight x=\"%g\" y=\"%g\" z=\"%g\" pointsAtX=\"%g\" pointsAtY=\"%g\" pointsAtZ=\"%g\"",
		l->x, l->y, l->z, l->pointsAtX, l->pointsAtY, l->pointsAtZ);
	if (sc->hasCone)
		fprintf(f, " limitingConeAngle=\"%g\"", l->limitingConeAngle);
	fprintf(f, " specularExponent=\"%g\"/>\n", l->specularExponent);
	fprintf(f, "    </feSpecularLighting>\n");
	fprintf(f, "    <feMerge><feMergeNode in=\"d\"/><feMergeNode in=\"s\"/></feMerge>\n");
	fprintf(f, "  </filter>\n");
	fprintf(f, "  <rect width=\"%d\" height=\"%d\" fill=\"black\" filter=\"url(#spot)\"/>\n", W, H);
	fprintf(f, "</svg>\n");
	fclose(f);
}

int main(void)
{
	SpotCase cases[4];
	int i;

	cases[0].name = "spot_front";
	cases[0].light = (NSVGspotLight){ 200, 200, 100, 200, 200, 0, 30.0f, 1.0f };
	cases[0].hasCone = 1;

	cases[1].name = "spot_oblique";
	cases[1].light = (NSVGspotLight){ 200, 200, 100, 260, 200, 0, 20.0f, 1.0f };
	cases[1].hasCone = 1;

	cases[2].name = "spot_narrow";
	cases[2].light = (NSVGspotLight){ 200, 200, 150, 200, 200, 0, 10.0f, 4.0f };
	cases[2].hasCone = 1;

	cases[3].name = "spot_regression";
	cases[3].light = (NSVGspotLight){ 200, 200, 120, 200, 200, 0, NAN, 1.0f };
	cases[3].hasCone = 0;

	for (i = 0; i < 4; i++) {
		char path[256];
		snprintf(path, sizeof(path), "%s_diffuse.png", cases[i].name);
		render(&cases[i], 0, path);
		snprintf(path, sizeof(path), "%s_specular.png", cases[i].name);
		render(&cases[i], 1, path);
		snprintf(path, sizeof(path), "%s.svg", cases[i].name);
		writeSvg(&cases[i], path);
		printf("rendered %s\n", cases[i].name);
	}
	return 0;
}
