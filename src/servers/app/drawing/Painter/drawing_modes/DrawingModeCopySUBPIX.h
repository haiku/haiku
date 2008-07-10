/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_COPY on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_COPY_SUBPIX_H
#define DRAWING_MODE_COPY_SUBPIX_H

#include "DrawingMode.h"

// BLEND_COPY_SUBPIX
#define BLEND_COPY_SUBPIX(d, r2, g2, b2, a1, a2, a3, r1, g1, b1) \
{ \
	BLEND_FROM_SUBPIX(d, r1, g1, b1, r2, g2, b2, a1, a2, a3); \
}


// BLEND_COPY
#define BLEND_COPY(d, r2, g2, b2, a, r1, g1, b1) \
{ \
	BLEND_FROM(d, r1, g1, b1, r2, g2, b2, a); \
}


// ASSIGN_COPY
#define ASSIGN_COPY(d, r, g, b, a) \
{ \
	d[0] = (b); \
	d[1] = (g); \
	d[2] = (r); \
	d[3] = (a); \
}


// blend_hline_copy_subpix
void
blend_hline_copy_subpix(int x, int y, unsigned len, const color_type& c,
	uint8 cover, agg_buffer* buffer, const PatternHandler* pattern)
{
	if (cover == 255) {
		// cache the low and high color as 32bit values
		// high color
		rgb_color color = pattern->HighColor();
		uint32 vh;
		uint8* p8 = (uint8*)&vh;
		p8[0] = (uint8)color.blue;
		p8[1] = (uint8)color.green;
		p8[2] = (uint8)color.red;
		p8[3] = 255;
		// low color
		color = pattern->LowColor();
		uint32 vl;
		p8 = (uint8*)&vl;
		p8[0] = (uint8)color.blue;
		p8[1] = (uint8)color.green;
		p8[2] = (uint8)color.red;
		p8[3] = 255;
		// row offset as 32bit pointer
		uint32* p32 = (uint32*)(buffer->row_ptr(y)) + x;
		do {
			if (pattern->IsHighColor(x, y))
				*p32 = vh;
			else
				*p32 = vl;
			p32++;
			x++;
			len -= 3;
		} while (len);
	} else {
		uint8* p = buffer->row_ptr(y) + (x << 2);
		rgb_color l = pattern->LowColor();
		do {
			rgb_color color = pattern->ColorAt(x, y);
			BLEND_COPY(p, color.red, color.green, color.blue, cover,
				l.red, l.green, l.blue);
			x++;
			p += 4;
			len -= 3;
		} while (len);
	}
}


// blend_solid_hspan_copy_subpix
void
blend_solid_hspan_copy_subpix(int x, int y, unsigned len, const color_type& c,
	const uint8* covers, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color l = pattern->LowColor();
	do {
		rgb_color color = pattern->ColorAt(x, y);
		BLEND_COPY_SUBPIX(p, color.red, color.green, color.blue,
			covers[2], covers[1], covers[0], l.red, l.green, l.blue);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}

#endif // DRAWING_MODE_COPY_SUBPIX_H

