/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_ALPHA in "Constant Overlay" mode on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_ALPHA_PO_SOLID_H
#define DRAWING_MODE_ALPHA_PO_SOLID_H

#include "DrawingModeAlphaPO.h"

// blend_pixel_alpha_po_solid
void
blend_pixel_alpha_po_solid(int x, int y, const color_type& c, uint8 cover,
						   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	uint16 alpha = c.a * cover;
	if (alpha == 255 * 255) {
		ASSIGN_ALPHA_PO(p, c.r, c.g, c.b);
	} else {
		BLEND_ALPHA_PO(p, c.r, c.g, c.b, alpha);
	}
}

// blend_hline_alpha_po_solid
void
blend_hline_alpha_po_solid(int x, int y, unsigned len, 
						   const color_type& c, uint8 cover,
						   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint16 alpha = c.a * cover;
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
		} while(--len);
	} else {
		uint8* p = buffer->row_ptr(y) + (x << 2);
		if (len < 4) {
			do {
				BLEND_ALPHA_CO(p, c.r, c.g, c.b, alpha);
				x++;
				p += 4;
			} while(--len);
		} else {
			alpha = alpha >> 8;
			blend_line32(p, len, c.r, c.g, c.b, alpha);
		}
	}
}

// blend_solid_hspan_alpha_po_solid
void
blend_solid_hspan_alpha_po_solid(int x, int y, unsigned len, 
								 const color_type& c, const uint8* covers,
						 		 agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		uint16 alpha = c.a * *covers;
		if (alpha) {
			if(alpha == 255 * 255) {
				ASSIGN_ALPHA_PO(p, c.r, c.g, c.b);
			} else {
				BLEND_ALPHA_PO(p, c.r, c.g, c.b, alpha);
			}
		}
		covers++;
		p += 4;
		x++;
	} while(--len);
}



// blend_solid_vspan_alpha_po_solid
void
blend_solid_vspan_alpha_po_solid(int x, int y, unsigned len, 
								 const color_type& c, const uint8* covers,
								 agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		uint16 alpha = c.a * *covers;
		if (alpha) {
			if (alpha == 255 * 255) {
				ASSIGN_ALPHA_PO(p, c.r, c.g, c.b);
			} else {
				BLEND_ALPHA_PO(p, c.r, c.g, c.b, alpha);
			}
		}
		covers++;
		p += buffer->stride();
		y++;
	} while(--len);
}

#endif // DRAWING_MODE_ALPHA_PO_SOLID_H

