/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_ALPHA in "Constant Composite" mode on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_ALPHA_CC_SUBPIX_H
#define DRAWING_MODE_ALPHA_CC_SUBPIX_H

#include "DrawingMode.h"
#include "GlobalSubpixelSettings.h"

// BLEND_ALPHA_CC_SUBPIX
#define BLEND_ALPHA_CC_SUBPIX(d, r, g, b, a1, a2, a3) \
{ \
	BLEND_COMPOSITE16_SUBPIX(d, r, g, b, a1, a2, a3); \
}


// blend_solid_hspan_alpha_cc_subpix
void
blend_solid_hspan_alpha_cc_subpix(int x, int y, unsigned len,
	const color_type& c, const uint8* covers, agg_buffer* buffer,
	const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	uint8 hAlpha = pattern->HighColor().alpha;
	uint16 alphaRed;
	uint16 alphaGreen;
	uint16 alphaBlue;
	const int subpixelL = gSubpixelOrderingRGB ? 2 : 0;
	const int subpixelM = 1;
	const int subpixelR = gSubpixelOrderingRGB ? 0 : 2;
	do {
		alphaRed = hAlpha * covers[subpixelL];
		alphaGreen = hAlpha * covers[subpixelM];
		alphaBlue = hAlpha * covers[subpixelR];
		rgb_color color = pattern->ColorAt(x, y);
		BLEND_ALPHA_CC_SUBPIX(p, color.red, color.green, color.blue,
			alphaBlue, alphaGreen, alphaRed);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}

#endif // DRAWING_MODE_ALPHA_CC_SUBPIX_H

