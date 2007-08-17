/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_COPY on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_COPY_H
#define DRAWING_MODE_COPY_H

#include "DrawingMode.h"

// BLEND_COPY
#define BLEND_COPY(d, r2, g2, b2, a, r1, g1, b1) \
{ \
	BLEND_FROM(d, r1, g1, b1, r2, g2, b2, a); \
}

// ASSIGN_COPY
#define ASSIGN_COPY(d, r, g, b, a) \
{ \
	d[0] = (b); \
	d[1] = (g); \
	d[2] = (r); \
	d[3] = (a); \
}


// blend_pixel_copy
void
blend_pixel_copy(int x, int y, const color_type& c, uint8 cover,
				 agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color color = pattern->ColorAt(x, y);
	if (cover == 255) {
		ASSIGN_COPY(p, color.red, color.green, color.blue, color.alpha);
	} else {
		rgb_color l = pattern->LowColor();
		BLEND_COPY(p, color.red, color.green, color.blue, cover,
				   l.red, l.green, l.blue);
	}
}

// blend_hline_copy
void
blend_hline_copy(int x, int y, unsigned len, 
						 const color_type& c, uint8 cover,
						 agg_buffer* buffer, const PatternHandler* pattern)
{
	if (cover == 255) {
		// cache the low and high color as 32bit values
		// high color
		rgb_color color = pattern->HighColor();
		uint32 vh;
		uint8* p8 = (uint8*)&vh;
		p8[0] = (uint8)color.blue;
		p8[1] = (uint8)color.green;
		p8[2] = (uint8)color.red;
		p8[3] = 255;
		// low color
		color = pattern->LowColor();
		uint32 vl;
		p8 = (uint8*)&vl;
		p8[0] = (uint8)color.blue;
		p8[1] = (uint8)color.green;
		p8[2] = (uint8)color.red;
		p8[3] = 255;
		// row offset as 32bit pointer
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
		rgb_color l = pattern->LowColor();
		do {
			rgb_color color = pattern->ColorAt(x, y);
			BLEND_COPY(p, color.red, color.green, color.blue, cover,
					   l.red, l.green, l.blue);
			x++;
			p += 4;
		} while(--len);
	}
}

// blend_solid_hspan_copy
void
blend_solid_hspan_copy(int x, int y, unsigned len, 
				  const color_type& c, const uint8* covers,
				  agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color l = pattern->LowColor();
	do {
		rgb_color color = pattern->ColorAt(x, y);
		if (*covers) {
			if (*covers == 255) {
				ASSIGN_COPY(p, color.red, color.green, color.blue, color.alpha);
			} else {
				BLEND_COPY(p, color.red, color.green, color.blue, *covers,
						   l.red, l.green, l.blue);
			}
		}
		covers++;
		p += 4;
		x++;
	} while(--len);
}



// blend_solid_vspan_copy
void
blend_solid_vspan_copy(int x, int y, unsigned len, 
					   const color_type& c, const uint8* covers,
					   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color l = pattern->LowColor();
	do {
		rgb_color color = pattern->ColorAt(x, y);
		if (*covers) {
			if (*covers == 255) {
				ASSIGN_COPY(p, color.red, color.green, color.blue, color.alpha);
			} else {
				BLEND_COPY(p, color.red, color.green, color.blue, *covers,
						   l.red, l.green, l.blue);
			}
		}
		covers++;
		p += buffer->stride();
		y++;
	} while(--len);
}


// blend_color_hspan_copy
void
blend_color_hspan_copy(int x, int y, unsigned len, const color_type* colors, 
					   const uint8* covers, uint8 cover,
					   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color l = pattern->LowColor();
	if (covers) {
		// non-solid opacity
		do {
			if(*covers) {
				if(*covers == 255) {
					ASSIGN_COPY(p, colors->r, colors->g, colors->b, colors->a);
				} else {
					BLEND_COPY(p, colors->r, colors->g, colors->b, *covers,
							   l.red, l.green, l.blue);
				}
			}
			covers++;
			p += 4;
			++colors;
		} while(--len);
	} else {
		// solid full opcacity
		if (cover == 255) {
			do {
				ASSIGN_COPY(p, colors->r, colors->g, colors->b, colors->a);
				p += 4;
				++colors;
			} while(--len);
		// solid partial opacity
		} else if (cover) {
			do {
				BLEND_COPY(p, colors->r, colors->g, colors->b, cover,
						   l.red, l.green, l.blue);
				p += 4;
				++colors;
			} while(--len);
		}
	}
}

#endif // DRAWING_MODE_COPY_H

