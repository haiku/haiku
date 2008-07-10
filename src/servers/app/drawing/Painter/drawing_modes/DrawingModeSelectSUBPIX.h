/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_SELECT on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_SELECT_SUBPIX_H
#define DRAWING_MODE_SELECT_SUBPIX_H

#include "DrawingMode.h"

// BLEND_SELECT_SUBPIX
#define BLEND_SELECT_SUBPIX(d, r, g, b, a1, a2, a3) \
{ \
	BLEND_SUBPIX(d, r, g, b, a1, a2, a3); \
}


// BLEND_SELECT
#define BLEND_SELECT(d, r, g, b, a) \
{ \
	BLEND(d, r, g, b, a); \
}


// ASSIGN_SELECT
#define ASSIGN_SELECT(d, r, g, b) \
{ \
	d[0] = (b); \
	d[1] = (g); \
	d[2] = (r); \
	d[3] = 255; \
}


// blend_hline_select_subpix
void
blend_hline_select_subpix(int x, int y, unsigned len, const color_type& c,
	uint8 cover, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color high = pattern->HighColor();
	rgb_color low = pattern->LowColor();
	rgb_color color;
	if (cover == 255) {
		do {
			if (pattern->IsHighColor(x, y)
				&& compare(p, high, low, &color))
				ASSIGN_SELECT(p, color.red, color.green, color.blue);
			x++;
			p += 4;
			len -= 3;
		} while (len);
	} else {
		do {
			if (pattern->IsHighColor(x, y)
				&& compare(p, high, low, &color)) {
				BLEND_SELECT(p, color.red, color.green, color.blue, cover);
			}
			x++;
			p += 4;
			len -= 3;
		} while (len);
	}
}


// blend_solid_hspan_select_subpix
void
blend_solid_hspan_select_subpix(int x, int y, unsigned len, const color_type& c,
	const uint8* covers, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color high = pattern->HighColor();
	rgb_color low = pattern->LowColor();
	rgb_color color;
	do {
		if (pattern->IsHighColor(x, y)) {
			if (compare(p, high, low, &color)){
				BLEND_SELECT_SUBPIX(p, color.red, color.green, color.blue,
					covers[2], covers[1], covers[0]);
			}
		}
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}

#endif // DRAWING_MODE_SELECT_SUBPIX_H

