/*
 * nanosvglight.h - SVG filter lighting primitives (feSpotLight + friends)
 *
 * Implements the light source and shading model used by the
 * feDiffuseLighting and feSpecularLighting filter primitives:
 *
 *   - feSpotLight: position (x,y,z), direction towards
 *     (pointsAtX,pointsAtY,pointsAtZ), optional limitingConeAngle and
 *     specularExponent controlling the cone edge falloff.
 *
 * Both shading primitives share the same cone attenuation term, so a
 * spot light clips and falls off identically whether it feeds a
 * diffuse or a specular primitive.
 *
 * Usage: in exactly one C/C++ file, define NANOSVGLIGHT_IMPLEMENTATION
 * before including this file to expand the implementation.
 *
 * References: https://www.w3.org/TR/SVG11/filters.html#feSpotLightElement
 */

#ifndef NANOSVGLIGHT_H
#define NANOSVGLIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NSVGspotLight {
	float x, y, z;                               /* light position */
	float pointsAtX, pointsAtY, pointsAtZ;       /* aim point */
	float limitingConeAngle;                     /* degrees; NAN = no cone limit */
	float specularExponent;                      /* cone edge falloff, default 1 */
} NSVGspotLight;

/* Spot attenuation in [0,1] for a surface point q: pow(-L.S, specularExponent)
 * per the SVG spec, where L points from q to the light and S from the light
 * to the aim point. Returns 0 when L.S is positive, and 0 outside the
 * limiting cone when limitingConeAngle is set. */
float nsvgSpotConeFactor(const NSVGspotLight* light, float qx, float qy, float qz);

/* Evaluate feDiffuseLighting with an feSpotLight source at surface point q
 * with unit normal n. kd is diffuseConstant. Result is written to rgb[3]. */
void nsvgDiffuseSpot(const NSVGspotLight* light,
					 float qx, float qy, float qz,
					 float nx, float ny, float nz,
					 float kd, const float lightColor[3], float rgb[3]);

/* Evaluate feSpecularLighting with an feSpotLight source at surface point q
 * with unit normal n. ks is the primitive's specularConstant and specExp the
 * primitive's specularExponent (distinct from the light's own exponent).
 * Result is written to rgb[3]. */
void nsvgSpecularSpot(const NSVGspotLight* light,
					  float qx, float qy, float qz,
					  float nx, float ny, float nz,
					  float ks, float specExp, const float lightColor[3], float rgb[3]);

#ifdef __cplusplus
}
#endif

#endif /* NANOSVGLIGHT_H */

#ifdef NANOSVGLIGHT_IMPLEMENTATION
#ifndef NANOSVGLIGHT_IMPLEMENTATION_ONCE
#define NANOSVGLIGHT_IMPLEMENTATION_ONCE

#include <math.h>

static float nsvg__clampf(float v, float lo, float hi)
{
	return v < lo ? lo : (v > hi ? hi : v);
}

static void nsvg__norm3(float* v)
{
	float len = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (len > 1e-12f) {
		v[0] /= len; v[1] /= len; v[2] /= len;
	}
}

/* Unit vector from surface point q towards the light position. */
static void nsvg__lightDir(const NSVGspotLight* light, float qx, float qy, float qz, float L[3])
{
	L[0] = light->x - qx;
	L[1] = light->y - qy;
	L[2] = light->z - qz;
	nsvg__norm3(L);
}

float nsvgSpotConeFactor(const NSVGspotLight* light, float qx, float qy, float qz)
{
	float L[3], S[3];
	float dot, cosLimit, exp;

	nsvg__lightDir(light, qx, qy, qz, L);

	/* Spot axis: from the light position towards the aim point. */
	S[0] = light->pointsAtX - light->x;
	S[1] = light->pointsAtY - light->y;
	S[2] = light->pointsAtZ - light->z;
	nsvg__norm3(S);

	dot = L[0]*S[0] + L[1]*S[1] + L[2]*S[2];
	dot = -dot; /* spec: attenuation is pow(-L.S, specularExponent) */

	/* Spec: if L.S is positive (dot <= 0 here), no light is present. */
	if (dot <= 0.0f)
		return 0.0f;

	if (!isnan(light->limitingConeAngle)) {
		cosLimit = cosf(light->limitingConeAngle * (float)M_PI / 180.0f);
		/* No light outside the limiting cone. */
		if (dot < cosLimit)
			return 0.0f;
	}

	exp = isnan(light->specularExponent) ? 1.0f : light->specularExponent;
	return powf(nsvg__clampf(dot, 0.0f, 1.0f), exp);
}

void nsvgDiffuseSpot(const NSVGspotLight* light,
					 float qx, float qy, float qz,
					 float nx, float ny, float nz,
					 float kd, const float lightColor[3], float rgb[3])
{
	float L[3], ndotl, cone, scale;
	int i;

	nsvg__lightDir(light, qx, qy, qz, L);
	ndotl = nsvg__clampf(nx*L[0] + ny*L[1] + nz*L[2], 0.0f, 1.0f);
	cone = nsvgSpotConeFactor(light, qx, qy, qz);
	scale = kd * ndotl * cone;
	for (i = 0; i < 3; i++)
		rgb[i] = lightColor[i] * scale;
}

void nsvgSpecularSpot(const NSVGspotLight* light,
					  float qx, float qy, float qz,
					  float nx, float ny, float nz,
					  float ks, float specExp, const float lightColor[3], float rgb[3])
{
	float L[3], H[3], ndoth, cone, scale;
	int i;

	nsvg__lightDir(light, qx, qy, qz, L);

	/* Half vector between L and the eye vector (0,0,1). */
	H[0] = L[0]; H[1] = L[1]; H[2] = L[2] + 1.0f;
	nsvg__norm3(H);

	ndoth = nsvg__clampf(nx*H[0] + ny*H[1] + nz*H[2], 0.0f, 1.0f);
	cone = nsvgSpotConeFactor(light, qx, qy, qz);
	scale = ks * powf(ndoth, specExp) * cone;
	for (i = 0; i < 3; i++)
		rgb[i] = lightColor[i] * scale;
}

#endif /* NANOSVGLIGHT_IMPLEMENTATION_ONCE */
#endif /* NANOSVGLIGHT_IMPLEMENTATION */
