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

// blend_hline_copy_text_subpix
void
blend_hline_copy_text_subpix(int x, int y, unsigned len, const color_type& c,
	uint8 cover, agg_buffer* buffer, const PatternHandler* pattern)
{
	if (cover == 255) {
		// cache the color as 32bit value
		uint32 v;
		uint8* p8 = (uint8*)&v;
		p8[0] = (uint8)c.b;
		p8[1] = (uint8)c.g;
		p8[2] = (uint8)c.r;
		p8[3] = 255;
		// row offset as 32bit pointer
		uint32* p32 = (uint32*)(buffer->row_ptr(y)) + x;
		do {
			*p32 = v;
			p32++;
			len -= 3;
		} while(len);
	} else {
		uint8* p = buffer->row_ptr(y) + (x << 2);
		rgb_color l = pattern->LowColor();
		do {
			BLEND_COPY(p, c.r, c.g, c.b, cover, l.red, l.green, l.blue);
			p += 4;
			len -= 3;
		} while (len);
	}
}


// blend_solid_hspan_copy_text_subpix
void
blend_solid_hspan_copy_text_subpix(int x, int y, unsigned len,
	const color_type& c, const uint8* covers, agg_buffer* buffer,
	const PatternHandler* pattern)
{
//printf("blend_solid_hspan_copy_text(%d, %d)\n", x, len);
	uint8* p = buffer->row_ptr(y) + (x << 2);
	//rgb_color l = pattern->LowColor();
	do {
		BLEND_OVER_SUBPIX(p, c.r, c.g, c.b, covers[2], covers[1], covers[0]);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}

#endif // DRAWING_MODE_COPY_TEXT_SUBPIX_H

