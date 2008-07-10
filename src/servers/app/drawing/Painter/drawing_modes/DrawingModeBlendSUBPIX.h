/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_ADD on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_BLEND_SUBPIX_H
#define DRAWING_MODE_BLEND_SUBPIX_H

#include "DrawingMode.h"


// BLEND_BLEND_SUBPIX
#define BLEND_BLEND_SUBPIX(d, r, g, b, a1, a2, a3) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	uint8 bt = (_p.data8[0] + (b)) >> 1; \
	uint8 gt = (_p.data8[1] + (g)) >> 1; \
	uint8 rt = (_p.data8[2] + (r)) >> 1; \
	BLEND_SUBPIX(d, rt, gt, bt, a1, a2, a3); \
}


// BLEND_BLEND
#define BLEND_BLEND(d, r, g, b, a) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	uint8 bt = (_p.data8[0] + (b)) >> 1; \
	uint8 gt = (_p.data8[1] + (g)) >> 1; \
	uint8 rt = (_p.data8[2] + (r)) >> 1; \
	BLEND(d, rt, gt, bt, a); \
}


// ASSIGN_BLEND
#define ASSIGN_BLEND(d, r, g, b) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	d[0] = (_p.data8[0] + (b)) >> 1; \
	d[1] = (_p.data8[1] + (g)) >> 1; \
	d[2] = (_p.data8[2] + (r)) >> 1; \
	d[3] = 255; \
}


// blend_hline_blend_subpix
void
blend_hline_blend_subpix(int x, int y, unsigned len, const color_type& c,
	uint8 cover, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (cover == 255) {
		do {
			rgb_color color = pattern->ColorAt(x, y);

			ASSIGN_BLEND(p, color.red, color.green, color.blue);

			p += 4;
			x++;
			len -= 3;
		} while (len);
	} else {
		do {
			rgb_color color = pattern->ColorAt(x, y);

			BLEND_BLEND(p, color.red, color.green, color.blue, cover);

			p += 4;
			x++;
			len -= 3;
		} while (len);
	}
}


// blend_solid_hspan_blend_subpix
void
blend_solid_hspan_blend_subpix(int x, int y, unsigned len, const color_type& c,
	const uint8* covers, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
		BLEND_BLEND_SUBPIX(p, color.red, color.green, color.blue,
			covers[2], covers[1], covers[0]);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}


#endif // DRAWING_MODE_BLEND_SUBPIX_H

