/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_ALPHA in "Constant Overlay" mode on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_ALPHA_CO_SOLID_SUBPIX_H
#define DRAWING_MODE_ALPHA_CO_SOLID_SUBPIX_H

#include "DrawingModeAlphaCOSUBPIX.h"
#include "GlobalSubpixelSettings.h"


// blend_solid_hspan_alpha_co_solid_subpix
void
blend_solid_hspan_alpha_co_solid_subpix(int x, int y, unsigned len,
	const color_type& c, const uint8* covers, agg_buffer* buffer,
	const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	uint8 hAlpha = pattern->HighColor().alpha;
	uint16 alphaL;
	uint16 alphaM;
	uint16 alphaR;
	const int subpixelL = gSubpixelOrderingRGB ? 2 : 0;
	const int subpixelM = 1;
	const int subpixelR = gSubpixelOrderingRGB ? 0 : 2;
	do {
		alphaL = hAlpha * covers[subpixelL];
		alphaM = hAlpha * covers[subpixelM];
		alphaR = hAlpha * covers[subpixelR];
		BLEND_ALPHA_CO_SUBPIX(p, c.r, c.g, c.b, alphaL, alphaM, alphaR);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}


#endif // DRAWING_MODE_ALPHA_CO_SOLID_SUBPIX_H

