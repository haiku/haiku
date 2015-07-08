/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2015, Augustin Cavalier <waddlesplash>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Effect from corTeX / Optimum.
 */

#include <SupportDefs.h>

#include "DrawStars.h"


// Hand-translated from x86 assembly.
/* draw a star (5 pixels) */
void draw_stars(int star_width, char* dstParam, char incParam)
{
	unsigned char* dst;
	unsigned char inc, al, cl;
	dst = (unsigned char*)dstParam;
	inc = (unsigned char)incParam;

	dst[star_width] += inc;
	if (dst[star_width] < inc) {
		// Overflowed.
		dst[star_width] = 255;
	}

	inc >>= 1;
	cl = dst[star_width - 1];
	cl += inc;
	if (cl < inc) {
		*dst = 255;
		dst[star_width - 1] = 255;
	} else {
		al = *dst;
		al += inc;
		if (al >= inc)
			*dst = al;
		else
			*dst = 255;
		dst[star_width - 1] = cl;
	}

	al = dst[star_width * 2];
	cl = dst[star_width + 1];
	al += inc;
	cl += inc;
	if (al < inc) {
		if (cl >= inc) {
			dst[star_width * 2] = 255;
			dst[star_width + 1] = cl;
			return;
		}
	} else {
		if (cl >= inc) {
			dst[star_width * 2] = al;
			dst[star_width + 1] = cl;
			return;
		}
	}

	dst[star_width * 2] = 255;
	dst[star_width + 1] = 255;
}
