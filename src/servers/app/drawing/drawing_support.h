/*
 * Copyright 2005-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef DRAWING_SUPPORT_H
#define DRAWING_SUPPORT_H


#include <SupportDefs.h>
#include <string.h>

class BRect;


// gfxset32
// * numBytes is expected to be a multiple of 4
static inline void
gfxset32(uint8* dst, uint32 color, int32 numBytes)
{
	uint64 s64 = ((uint64)color << 32) | color;
	while (numBytes >= 8) {
		*(uint64*)dst = s64;
		numBytes -= 8;
		dst += 8;
	}
	if (numBytes == 4) {
		*(uint32*)dst = color;
	}
}

union pixel32 {
	uint32	data32;
	uint8	data8[4];
};

// blend_line32
static inline void
blend_line32(uint8* buffer, int32 pixels, uint8 r, uint8 g, uint8 b, uint8 a)
{
	pixel32 p;

	r = (r * a) >> 8;
	g = (g * a) >> 8;
	b = (b * a) >> 8;
	a = 255 - a;

	uint8 tempBuffer[pixels * 4];

	uint8* t = tempBuffer;
	uint8* s = buffer;

	for (int32 i = 0; i < pixels; i++) {
		p.data32 = *(uint32*)s;

		t[0] = ((p.data8[0] * a) >> 8) + b;
		t[1] = ((p.data8[1] * a) >> 8) + g;
		t[2] = ((p.data8[2] * a) >> 8) + r;

		t += 4;
		s += 4;
	}

	memcpy(buffer, tempBuffer, pixels * 4);
}

void align_rect_to_pixels(BRect* rect);

#endif	// DRAWING_SUPPORT_H
