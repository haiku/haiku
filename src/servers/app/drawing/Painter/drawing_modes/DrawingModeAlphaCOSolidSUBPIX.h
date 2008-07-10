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

// blend_hline_alpha_co_solid_subpix
void
blend_hline_alpha_co_solid_subpix(int x, int y, unsigned len,
	const color_type& c, uint8 cover, agg_buffer* buffer,
	const PatternHandler* pattern)
{
	uint16 alpha = pattern->HighColor().alpha * cover;
	if (alpha == 255 * 255) {
		// cache the color as 32bit values
		uint32 v;
		uint8* p8 = (uint8*)&v;
		p8[0] = c.b;
		p8[1] = c.g;
		p8[2] = c.r;
		p8[3] = 255;
		// row offset as 32bit pointer
		uint32* p32 = (uint32*)(buffer->row_ptr(y)) + x;
		do {
			*p32 = v;
			p32++;
			x++;
			len -= 3;
		} while (len);
	} else {
		uint8* p = buffer->row_ptr(y) + (x << 2);
		if (len < 12) {
			do {
				BLEND_ALPHA_CO(p, c.r, c.g, c.b, alpha);
				x++;
				p += 4;
				len -= 3;
			} while (len);
		} else {
			alpha = alpha >> 8;
			blend_line32(p, len / 3, c.r, c.g, c.b, alpha);
		}
	}
}


// blend_solid_hspan_alpha_co_solid_subpix
void
blend_solid_hspan_alpha_co_solid_subpix(int x, int y, unsigned len,
	const color_type& c, const uint8* covers, agg_buffer* buffer,
	const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	uint8 hAlpha = pattern->HighColor().alpha;
	uint16 alphaRed;
	uint16 alphaGreen;
	uint16 alphaBlue;
	do {
		alphaRed = hAlpha * covers[0];
		alphaGreen = hAlpha * covers[1];
		alphaBlue = hAlpha * covers[2];
		BLEND_ALPHA_CO_SUBPIX(p, c.r, c.g, c.b,
			alphaBlue, alphaGreen, alphaRed);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}


#endif // DRAWING_MODE_ALPHA_CO_SOLID_SUBPIX_H

