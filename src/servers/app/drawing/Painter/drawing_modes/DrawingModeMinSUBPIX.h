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
#include "GlobalSubpixelSettings.h"

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


// blend_solid_hspan_min_subpix
void
blend_solid_hspan_min_subpix(int x, int y, unsigned len, const color_type& c,
	const uint8* covers, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	const int subpixelL = gSubpixelOrderingRGB ? 2 : 0;
	const int subpixelM = 1;
	const int subpixelR = gSubpixelOrderingRGB ? 0 : 2;
	do {
		rgb_color color = pattern->ColorAt(x, y);
		BLEND_MIN_SUBPIX(p, color.red, color.green, color.blue,
			covers[subpixelL], covers[subpixelM], covers[subpixelR]);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}

#endif // DRAWING_MODE_MIN_SUBPIX_H

