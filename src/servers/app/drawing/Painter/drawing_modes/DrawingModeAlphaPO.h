/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_ALPHA in "Constant Overlay" mode on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_ALPHA_PO_H
#define DRAWING_MODE_ALPHA_PO_H

#include "DrawingMode.h"

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

// blend_pixel_alpha_po
void
blend_pixel_alpha_po(int x, int y, const color_type& c, uint8 cover,
					 agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color color = pattern->ColorAt(x, y);
	uint16 alpha = color.alpha * cover;
	if (alpha == 255 * 255) {
		ASSIGN_ALPHA_PO(p, color.red, color.green, color.blue);
	} else {
		BLEND_ALPHA_PO(p, color.red, color.green, color.blue, alpha);
	}
}

// blend_hline_alpha_po
void
blend_hline_alpha_po(int x, int y, unsigned len, 
					 const color_type& c, uint8 cover,
					 agg_buffer* buffer, const PatternHandler* pattern)
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
	} while(--len);
}

// blend_solid_hspan_alpha_po
void
blend_solid_hspan_alpha_po(int x, int y, unsigned len, 
						   const color_type& c, const uint8* covers,
						   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
		uint16 alpha = color.alpha * *covers;
		if (alpha) {
			if(alpha == 255 * 255) {
				ASSIGN_ALPHA_PO(p, color.red, color.green, color.blue);
			} else {
				BLEND_ALPHA_PO(p, color.red, color.green, color.blue, alpha);
			}
		}
		covers++;
		p += 4;
		x++;
	} while(--len);
}



// blend_solid_vspan_alpha_po
void
blend_solid_vspan_alpha_po(int x, int y, unsigned len, 
						   const color_type& c, const uint8* covers,
						   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
		uint16 alpha = color.alpha * *covers;
		if (alpha) {
			if (alpha == 255 * 255) {
				ASSIGN_ALPHA_PO(p, color.red, color.green, color.blue);
			} else {
				BLEND_ALPHA_PO(p, color.red, color.green, color.blue, alpha);
			}
		}
		covers++;
		p += buffer->stride();
		y++;
	} while(--len);
}


// blend_color_hspan_alpha_po
void
blend_color_hspan_alpha_po(int x, int y, unsigned len, 
						   const color_type* colors, 
						   const uint8* covers, uint8 cover,
						   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (covers) {
		// non-solid opacity
		do {
			uint16 alpha = colors->a * *covers;
			if (alpha) {
				if (alpha == 255 * 255) {
					ASSIGN_ALPHA_PO(p, colors->r, colors->g, colors->b);
				} else {
					BLEND_ALPHA_PO(p, colors->r, colors->g, colors->b, alpha);
				}
			}
			covers++;
			p += 4;
			++colors;
		} while(--len);
	} else {
		// solid full opcacity
		uint16 alpha = colors->a * cover;
		if (alpha == 255 * 255) {
			do {
				ASSIGN_ALPHA_PO(p, colors->r, colors->g, colors->b);
				p += 4;
				++colors;
			} while(--len);
		// solid partial opacity
		} else if (alpha) {
			do {
				BLEND_ALPHA_PO(p, colors->r, colors->g, colors->b, alpha);
				p += 4;
				++colors;
			} while(--len);
		}
	}
}

#endif // DRAWING_MODE_ALPHA_PO_H

