/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_SUBTRACT on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_SUBTRACT_H
#define DRAWING_MODE_SUBTRACT_H

#include <SupportDefs.h>

#include "DrawingMode.h"
#include "PatternHandler.h"

// BLEND_SUBTRACT
#define BLEND_SUBTRACT(d, r, g, b, a) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	uint8 rt = max_c(0, _p.data8[2] - (r)); \
	uint8 gt = max_c(0, _p.data8[1] - (g)); \
	uint8 bt = max_c(0, _p.data8[0] - (b)); \
	BLEND(d, rt, gt, bt, a); \
}

// ASSIGN_SUBTRACT
#define ASSIGN_SUBTRACT(d, r, g, b) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	d[0] = max_c(0, _p.data8[0] - (b)); \
	d[1] = max_c(0, _p.data8[1] - (g)); \
	d[2] = max_c(0, _p.data8[2] - (r)); \
	d[3] = 255; \
}


// blend_pixel_subtract
void
blend_pixel_subtract(int x, int y, const color_type& c, uint8 cover,
					 agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color color = pattern->ColorAt(x, y);
	if (cover == 255) {
		ASSIGN_SUBTRACT(p, color.red, color.green, color.blue);
	} else {
		BLEND_SUBTRACT(p, color.red, color.green, color.blue, cover);
	}
}

// blend_hline_subtract
void
blend_hline_subtract(int x, int y, unsigned len, 
					 const color_type& c, uint8 cover,
					 agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (cover == 255) {
		do {
			rgb_color color = pattern->ColorAt(x, y);

			ASSIGN_SUBTRACT(p, color.red, color.green, color.blue);

			p += 4;
			x++;
		} while(--len);
	} else {
		do {
			rgb_color color = pattern->ColorAt(x, y);

			BLEND_SUBTRACT(p, color.red, color.green, color.blue, cover);

			x++;
			p += 4;
		} while(--len);
	}
}

// blend_solid_hspan_subtract
void
blend_solid_hspan_subtract(int x, int y, unsigned len, 
						   const color_type& c, const uint8* covers,
						   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
		if (*covers) {
			if (*covers == 255) {
				ASSIGN_SUBTRACT(p, color.red, color.green, color.blue);
			} else {
				BLEND_SUBTRACT(p, color.red, color.green, color.blue, *covers);
			}
		}
		covers++;
		p += 4;
		x++;
	} while(--len);
}



// blend_solid_vspan_subtract
void
blend_solid_vspan_subtract(int x, int y, unsigned len, 
						   const color_type& c, const uint8* covers,
						   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
		if (*covers) {
			if (*covers == 255) {
				ASSIGN_SUBTRACT(p, color.red, color.green, color.blue);
			} else {
				BLEND_SUBTRACT(p, color.red, color.green, color.blue, *covers);
			}
		}
		covers++;
		p += buffer->stride();
		y++;
	} while(--len);
}


// blend_color_hspan_subtract
void
blend_color_hspan_subtract(int x, int y, unsigned len, 
						   const color_type* colors, 
						   const uint8* covers, uint8 cover,
						   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (covers) {
		// non-solid opacity
		do {
			if (*covers && colors->a > 0) {
				if (*covers == 255) {
					ASSIGN_SUBTRACT(p, colors->r, colors->g, colors->b);
				} else {
					BLEND_SUBTRACT(p, colors->r, colors->g, colors->b, *covers);
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
				if (colors->a > 0) {
					ASSIGN_SUBTRACT(p, colors->r, colors->g, colors->b);
				}
				p += 4;
				++colors;
			} while(--len);
		// solid partial opacity
		} else if (cover) {
			do {
				if (colors->a > 0) {
					BLEND_SUBTRACT(p, colors->r, colors->g, colors->b, cover);
				}
				p += 4;
				++colors;
			} while(--len);
		}
	}
}

#endif // DRAWING_MODE_SUBTRACT_H

