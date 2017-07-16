/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2015, Augustin Cavalier <waddlesplash>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Effect from corTeX / Optimum.
 */

#include <SupportDefs.h>

#include "Draw.h"


// Hand-translated from x86 assembly.
/* memshset (char *dst, int center_shade,int fixed_shade, int length_2) */
/* dst is the *center* of the dst segment */
/* center shade is the initial color (at center) (!! 8:8 also) */
/* fixed_shade is the fixed point increment (8:8) */
/* length_2 is the 1/2 length to set (to the left, and to the right) */

/* a memaddshadedset function (or something like that ) */
void memshset(char* dstParam, int center_shade, int fixed_shade, int half_length)
{
	unsigned char* dst;
	unsigned char* source;
	int offset;

	if (half_length == 0)
		return;

	dst = (unsigned char*)dstParam;
	source = dst;
	dst -= half_length; // Left segment
	source += half_length; // Right segment
	source++; // Don't overlap at middle

	offset = half_length;
	offset = -offset;
	offset--; // For in-advance incl in loop

	do {
		int tmp;
		center_shade += fixed_shade; // Increment color.
		center_shade &= 0xFFFF; // Mask so we can do overflow tests.
		offset++;

		tmp = dst[half_length] + (center_shade >> 8); // Increment left pixel.
		// Did it overflow?
		if (tmp & 0xFF00)
			dst[half_length] = 255;
		else
			dst[half_length] = tmp;

		tmp = source[offset] + (center_shade >> 8); // Increment right pixel.
		if (tmp & 0xFF00)
			source[offset] = 255;
		else
			source[offset] = tmp;
	} while (--half_length != 0);
}


/* do the "motion blur" (actually the pixel fading) */
void mblur(char* srcParam, int nbpixels)
{
	unsigned int clear1UpperBit = 0x7f7f7f7f;
	unsigned int clear3UpperBits = 0x1f1f1f1f;
	unsigned int* src = (unsigned int*)srcParam;

	if ((nbpixels >>= 2) == 0)
		return;

	do {
		unsigned int eax, ebx;
		eax = *src;

		ebx = eax;
		eax >>= 1;

		ebx >>= 3;
		eax &= clear1UpperBit;

		ebx &= clear3UpperBits;

		eax += ebx;
		*src = eax;
		src++;
	} while (--nbpixels != 0);
}
