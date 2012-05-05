/*
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 */
#ifndef CONVERT_RGB_HSV_H
#define CONVERT_RGB_HSV_H


#include <math.h>


#define RETURN_HSV(h, s, v) { H = h; S = s; V = v; return; }
#define RETURN_RGB(r, g, b) { R = r; G = g; B = b; return; }

#define UNDEFINED 0


inline void
RGB_to_HSV(float R, float G, float B, float& H, float& S, float& V)
{
	// RGB are each on [0, 1]. S and max are returned on [0, 1] and H is
	// returned on [0, 6]. Exception: H is returned UNDEFINED if S==0.

	float min, max;

	if (R > G) {
		if (R > B) {
			max = R;
			min = G > B ? B : G;
		} else {
			max = B;
			min = G;
		}
	}
	else {
		if (G > B) {
			max = G;
			min = R > B ? B : R;
		} else {
			max = B;
			min = R;
		}
	}

	if (max == min)
		RETURN_HSV(UNDEFINED, 0, max);

	float dist = max - min;

	float f = (R == min)
		? G - B
		: ((G == min) ? B - R : R - G);
	float i = (R == min)
		? 3
		: ((G == min) ? 5 : 1);
	float h = i - f / dist;

	while (h >= 6.0)
		h -= 6.0;

	RETURN_HSV(h, dist/max, max);
}


inline void
HSV_to_RGB(float h, float s, float v, float& R, float& G, float& B)
{
	// H is given on [0, 6] or UNDEFINED. S and V are given on [0, 1].
	// RGB are each returned on [0, 1].

	int i = (int)floor(h);

	float f = h - i;

	if ( !(i & 1) )
		f = 1 - f; // if i is even

	float m = v * (1 - s);

	float n = v * (1 - s * f);

	switch (i) {
		case 6:
		case 0:
			RETURN_RGB(v, n, m);

		case 1:
			RETURN_RGB(n, v, m);

		case 2:
			RETURN_RGB(m, v, n)

		case 3:
			RETURN_RGB(m, n, v);

		case 4:
			RETURN_RGB(n, m, v);

		case 5:
			RETURN_RGB(v, m, n);

		default:
			RETURN_RGB(0, 0, 0);
	}
}


#endif // CONVERT_RGB_HSV_H
