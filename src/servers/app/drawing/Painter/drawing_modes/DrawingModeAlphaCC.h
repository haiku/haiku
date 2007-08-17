/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_ALPHA in "Constant Composite" mode on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_ALPHA_CC_H
#define DRAWING_MODE_ALPHA_CC_H

#include "DrawingMode.h"

// BLEND_ALPHA_CC
#define BLEND_ALPHA_CC(d, r, g, b, a) \
{ \
	BLEND_COMPOSITE16(d, r, g, b, a); \
}

// ASSIGN_ALPHA_CC
#define ASSIGN_ALPHA_CC(d, r, g, b) \
{ \
	d[0] = (b); \
	d[1] = (g); \
	d[2] = (r); \
	d[3] = 255; \
}

// blend_pixel_alpha_cc
void
blend_pixel_alpha_cc(int x, int y, const color_type& c, uint8 cover,
					 agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color color = pattern->ColorAt(x, y);
	uint16 alpha = pattern->HighColor().alpha * cover;
	if (alpha == 255 * 255) {
		ASSIGN_ALPHA_CC(p, color.red, color.green, color.blue);
	} else {
		BLEND_ALPHA_CC(p, color.red, color.green, color.blue, alpha);
	}
}

// blend_hline_alpha_cc
void
blend_hline_alpha_cc(int x, int y, unsigned len, 
					 const color_type& c, uint8 cover,
					 agg_buffer* buffer, const PatternHandler* pattern)
{
	rgb_color color = pattern->HighColor();
	uint16 alpha = color.alpha * cover;
	if (alpha == 255 * 255) {
		// cache the low and high color as 32bit values
		// high color
		uint32 vh;
		uint8* p8 = (uint8*)&vh;
		p8[0] = color.blue;
		p8[1] = color.green;
		p8[2] = color.red;
		p8[3] = 255;
		// low color
		color = pattern->LowColor();
		uint32 vl;
		p8 = (uint8*)&vl;
		p8[0] = color.blue;
		p8[1] = color.green;
		p8[2] = color.red;
		p8[3] = 255;
		// row offset as 32 bit pointer
		uint32* p32 = (uint32*)(buffer->row_ptr(y)) + x;
		do {
			if (pattern->IsHighColor(x, y))
				*p32 = vh;
			else
				*p32 = vl;
			p32++;
			x++;
		} while(--len);
	} else {
		uint8* p = buffer->row_ptr(y) + (x << 2);
		do {
			rgb_color color = pattern->ColorAt(x, y);
			BLEND_ALPHA_CC(p, color.red, color.green, color.blue, alpha);
			x++;
			p += 4;
		} while(--len);
	}
}

// blend_solid_hspan_alpha_cc
void
blend_solid_hspan_alpha_cc(int x, int y, unsigned len, 
						   const color_type& c, const uint8* covers,
						   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	uint8 hAlpha = pattern->HighColor().alpha;
	do {
		rgb_color color = pattern->ColorAt(x, y);
		uint16 alpha = hAlpha * *covers;
		if (alpha) {
			if (alpha == 255 * 255) {
				ASSIGN_ALPHA_CC(p, color.red, color.green, color.blue);
			} else {
				BLEND_ALPHA_CC(p, color.red, color.green, color.blue, alpha);
			}
		}
		covers++;
		p += 4;
		x++;
	} while(--len);
}

// blend_solid_vspan_alpha_cc
void
blend_solid_vspan_alpha_cc(int x, int y, unsigned len, 
						   const color_type& c, const uint8* covers,
						   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	uint8 hAlpha = pattern->HighColor().alpha;
	do {
		rgb_color color = pattern->ColorAt(x, y);
		uint16 alpha = hAlpha * *covers;
		if (alpha) {
			if (alpha == 255 * 255) {
				ASSIGN_ALPHA_CC(p, color.red, color.green, color.blue);
			} else {
				BLEND_ALPHA_CC(p, color.red, color.green, color.blue, alpha);
			}
		}
		covers++;
		p += buffer->stride();
		y++;
	} while(--len);
}


// blend_color_hspan_alpha_cc
void
blend_color_hspan_alpha_cc(int x, int y, unsigned len, 
						   const color_type* colors, 
						   const uint8* covers, uint8 cover,
						   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	uint8 hAlpha = pattern->HighColor().alpha;
	if (covers) {
		// non-solid opacity
		do {
			uint16 alpha = hAlpha * colors->a * *covers / 255;
			if (alpha) {
				if (alpha == 255 * 255) {
					ASSIGN_ALPHA_CC(p, colors->r, colors->g, colors->b);
				} else {
					BLEND_ALPHA_CC(p, colors->r, colors->g, colors->b, alpha);
				}
			}
			covers++;
			p += 4;
			++colors;
		} while(--len);
	} else {
		// solid full opcacity
		uint16 alpha = hAlpha * colors->a * cover / 255;
		if (alpha == 255 * 255) {
			do {
				ASSIGN_ALPHA_CC(p, colors->r, colors->g, colors->b);
				p += 4;
				++colors;
			} while(--len);
		// solid partial opacity
		} else if (alpha) {
			do {
				BLEND_ALPHA_CC(p, colors->r, colors->g, colors->b, alpha);
				p += 4;
				++colors;
			} while(--len);
		}
	}
}

#endif // DRAWING_MODE_ALPHA_CC_H

