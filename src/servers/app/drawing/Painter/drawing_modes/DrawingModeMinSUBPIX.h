/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_MIN on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_MIN_SUBPIX_H
#define DRAWING_MODE_MIN_SUBPIX_H

#include "DrawingMode.h"

// BLEND_MIN_SUBPIX
#define BLEND_MIN_SUBPIX(d, r, g, b, a1, a2, a3) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	if (brightness_for((r), (g), (b)) \
		< brightness_for(_p.data8[2], _p.data8[1], _p.data8[0])) { \
		BLEND_SUBPIX(d, (r), (g), (b), a1, a2, a3); \
	} \
}


// BLEND_MIN
#define BLEND_MIN(d, r, g, b, a) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	if (brightness_for((r), (g), (b)) \
		< brightness_for(_p.data8[2], _p.data8[1], _p.data8[0])) { \
		BLEND(d, (r), (g), (b), a); \
	} \
}


// ASSIGN_MIN
#define ASSIGN_MIN(d, r, g, b) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	if (brightness_for((r), (g), (b)) \
		< brightness_for(_p.data8[2], _p.data8[1], _p.data8[0])) { \
		d[0] = (b); \
		d[1] = (g); \
		d[2] = (r); \
		d[3] = 255; \
	} \
}


// blend_hline_min_subpix
void
blend_hline_min_subpix(int x, int y, unsigned len, const color_type& c,
	uint8 cover, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (cover == 255) {
		do {
			rgb_color color = pattern->ColorAt(x, y);

			ASSIGN_MIN(p, color.red, color.green, color.blue);

			p += 4;
			x++;
			len -= 3;
		} while (len);
	} else {
		do {
			rgb_color color = pattern->ColorAt(x, y);

			BLEND_MIN(p, color.red, color.green, color.blue, cover);

			x++;
			p += 4;
			len -= 3;
		} while (len);
	}
}


// blend_solid_hspan_min_subpix
void
blend_solid_hspan_min_subpix(int x, int y, unsigned len, const color_type& c,
	const uint8* covers, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
		BLEND_MIN_SUBPIX(p, color.red, color.green, color.blue,
			covers[2], covers[1], covers[0]);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}

#endif // DRAWING_MODE_MIN_SUBPIX_H

