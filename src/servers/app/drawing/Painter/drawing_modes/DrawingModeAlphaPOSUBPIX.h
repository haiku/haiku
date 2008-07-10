/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_ALPHA in "Constant Overlay" mode on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_ALPHA_PO_SUBPIX_H
#define DRAWING_MODE_ALPHA_PO_SUBPIX_H

#include "DrawingMode.h"

// BLEND_ALPHA_PO_SUBPIX
#define BLEND_ALPHA_PO_SUBPIX(d, r, g, b, a1, a2, a3) \
{ \
	BLEND16_SUBPIX(d, r, g, b, a1, a2, a3); \
}

// BLEND_ALPHA_PO
#define BLEND_ALPHA_PO(d, r, g, b, a) \
{ \
	BLEND16(d, r, g, b, a); \
}

// ASSIGN_ALPHA_PO
#define ASSIGN_ALPHA_PO(d, r, g, b) \
{ \
	d[0] = (b); \
	d[1] = (g); \
	d[2] = (r); \
	d[3] = 255; \
}


// blend_hline_alpha_po_subpix
void
blend_hline_alpha_po_subpix(int x, int y, unsigned len, const color_type& c,
	uint8 cover, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
		uint16 alpha = color.alpha * cover;
		if (alpha) {
			if (alpha == 255) {
				ASSIGN_ALPHA_PO(p, color.red, color.green, color.blue);
			} else {
				BLEND_ALPHA_PO(p, color.red, color.green, color.blue, alpha);
			}
		}
		x++;
		p += 4;
		len -= 3;
	} while(len);
}


// blend_solid_hspan_alpha_po_subpix
void
blend_solid_hspan_alpha_po_subpix(int x, int y, unsigned len,
	const color_type& c, const uint8* covers, agg_buffer* buffer,
	const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	uint16 alphaRed;
	uint16 alphaGreen;
	uint16 alphaBlue;
	do {
		rgb_color color = pattern->ColorAt(x, y);
		alphaRed = color.alpha * covers[0];
		alphaGreen = color.alpha * covers[1];
		alphaBlue = color.alpha * covers[2];
		BLEND_ALPHA_PO_SUBPIX(p, color.red, color.green, color.blue,
			alphaBlue, alphaGreen, alphaRed);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}

#endif // DRAWING_MODE_ALPHA_PO_SUBPIX_H

