/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_COPY for text on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_COPY_TEXT_SUBPIX_H
#define DRAWING_MODE_COPY_TEXT_SUBPIX_H

#include "DrawingModeCopySUBPIX.h"
#include "GlobalSubpixelSettings.h"

// blend_solid_hspan_copy_text_subpix
void
blend_solid_hspan_copy_text_subpix(int x, int y, unsigned len,
	const color_type& c, const uint8* covers, agg_buffer* buffer,
	const PatternHandler* pattern)
{
//printf("blend_solid_hspan_copy_text(%d, %d)\n", x, len);
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color l = pattern->LowColor();
	const int subpixelL = gSubpixelOrderingRGB ? 2 : 0;
	const int subpixelM = 1;
	const int subpixelR = gSubpixelOrderingRGB ? 0 : 2;
	do {
		BLEND_COPY_SUBPIX(p, c.r, c.g, c.b, covers[subpixelL],
			covers[subpixelM], covers[subpixelR], l.red, l.green, l.blue);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}

#endif // DRAWING_MODE_COPY_TEXT_SUBPIX_H

