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
#include "GlobalSubpixelSettings.h"

// BLEND_COPY_SUBPIX
#define BLEND_COPY_SUBPIX(d, r2, g2, b2, a1, a2, a3, r1, g1, b1) \
{ \
	BLEND_FROM_SUBPIX(d, r1, g1, b1, r2, g2, b2, a1, a2, a3); \
}


// blend_solid_hspan_copy_subpix
void
blend_solid_hspan_copy_subpix(int x, int y, unsigned len, const color_type& c,
	const uint8* covers, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color l = pattern->LowColor();
	const int subpixelL = gSubpixelOrderingRGB ? 2 : 0;
	const int subpixelM = 1;
	const int subpixelR = gSubpixelOrderingRGB ? 0 : 2;
	do {
		rgb_color color = pattern->ColorAt(x, y);
		BLEND_COPY_SUBPIX(p, color.red, color.green, color.blue,
			covers[subpixelL], covers[subpixelM], covers[subpixelR], l.red,
			l.green, l.blue);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}

#endif // DRAWING_MODE_COPY_SUBPIX_H

