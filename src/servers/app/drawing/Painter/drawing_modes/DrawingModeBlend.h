/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_BLEND on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_BLEND_H
#define DRAWING_MODE_BLEND_H

#include "DrawingMode.h"

// BLEND_BLEND
#define BLEND_BLEND(d, r, g, b, a) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	uint8 bt = (_p.data8[0] + (b)) >> 1; \
	uint8 gt = (_p.data8[1] + (g)) >> 1; \
	uint8 rt = (_p.data8[2] + (r)) >> 1; \
	BLEND(d, rt, gt, bt, a); \
}

// ASSIGN_BLEND
#define ASSIGN_BLEND(d, r, g, b) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	d[0] = (_p.data8[0] + (b)) >> 1; \
	d[1] = (_p.data8[1] + (g)) >> 1; \
	d[2] = (_p.data8[2] + (r)) >> 1; \
	d[3] = 255; \
}

// blend_pixel_blend
void
blend_pixel_blend(int x, int y, const color_type& c, uint8 cover,
				  agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color color = pattern->ColorAt(x, y);
	if (cover == 255) {
		ASSIGN_BLEND(p, color.red, color.green, color.blue);
	} else {
		BLEND_BLEND(p, color.red, color.green, color.blue, cover);
	}
}

// blend_hline_blend
void
blend_hline_blend(int x, int y, unsigned len, 
				  const color_type& c, uint8 cover,
				  agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (cover == 255) {
		do {
			rgb_color color = pattern->ColorAt(x, y);

			ASSIGN_BLEND(p, color.red, color.green, color.blue);

			p += 4;
			x++;
		} while(--len);
	} else {
		do {
			rgb_color color = pattern->ColorAt(x, y);

			BLEND_BLEND(p, color.red, color.green, color.blue, cover);

			x++;
			p += 4;
		} while(--len);
	}
}

// blend_solid_hspan_blend
void
blend_solid_hspan_blend(int x, int y, unsigned len, 
						const color_type& c, const uint8* covers,
						agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
		if (*covers) {
			if (*covers == 255) {
				ASSIGN_BLEND(p, color.red, color.green, color.blue);
			} else {
				BLEND_BLEND(p, color.red, color.green, color.blue, *covers);
			}
		}
		covers++;
		p += 4;
		x++;
	} while(--len);
}



// blend_solid_vspan_blend
void
blend_solid_vspan_blend(int x, int y, unsigned len, 
						const color_type& c, const uint8* covers,
						agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
		if (*covers) {
			if (*covers == 255) {
				ASSIGN_BLEND(p, color.red, color.green, color.blue);
			} else {
				BLEND_BLEND(p, color.red, color.green, color.blue, *covers);
			}
		}
		covers++;
		p += buffer->stride();
		y++;
	} while(--len);
}


// blend_color_hspan_blend
void
blend_color_hspan_blend(int x, int y, unsigned len, 
						const color_type* colors, 
						const uint8* covers, uint8 cover,
						agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (covers) {
		// non-solid opacity
		do {
//			if (*covers) {
if (*covers && (colors->a & 0xff)) {
				if (*covers == 255) {
					ASSIGN_BLEND(p, colors->r, colors->g, colors->b);
				} else {
					BLEND_BLEND(p, colors->r, colors->g, colors->b, *covers);
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
if (colors->a & 0xff) {
				ASSIGN_BLEND(p, colors->r, colors->g, colors->b);
}
				p += 4;
				++colors;
			} while(--len);
		// solid partial opacity
		} else if (cover) {
			do {
				BLEND_BLEND(p, colors->r, colors->g, colors->b, cover);
				p += 4;
				++colors;
			} while(--len);
		}
	}
}

#endif // DRAWING_MODE_BLEND_H

