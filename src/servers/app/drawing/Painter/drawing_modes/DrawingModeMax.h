/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_MAX on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_MAX_H
#define DRAWING_MODE_MAX_H

#include "DrawingMode.h"


// BLEND_MAX
#define BLEND_MAX(d, r, g, b, a) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	uint8 rt = max_c(_p.data8[2], (r)); \
	uint8 gt = max_c(_p.data8[1], (g)); \
	uint8 bt = max_c(_p.data8[0], (b)); \
	BLEND(d, rt, gt, bt, a); \
}

// ASSIGN_MAX
#define ASSIGN_MAX(d, r, g, b) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	d[0] = max_c(_p.data8[0], (b)); \
	d[1] = max_c(_p.data8[1], (g)); \
	d[2] = max_c(_p.data8[2], (r)); \
	d[3] = 255; \
}


// blend_pixel_max
void
blend_pixel_max(int x, int y, const color_type& c, uint8 cover,
				agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row(y) + (x << 2);
	rgb_color color = pattern->R5ColorAt(x, y);
	if (cover == 255) {
		ASSIGN_MAX(p, color.red, color.green, color.blue);
	} else {
		BLEND_MAX(p, color.red, color.green, color.blue, cover);
	}
}

// blend_hline_max
void
blend_hline_max(int x, int y, unsigned len, 
				const color_type& c, uint8 cover,
				agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row(y) + (x << 2);
	if (cover == 255) {
		do {
			rgb_color color = pattern->R5ColorAt(x, y);

			ASSIGN_MAX(p, color.red, color.green, color.blue);

			p += 4;
			x++;
		} while(--len);
	} else {
		do {
			rgb_color color = pattern->R5ColorAt(x, y);

			BLEND_MAX(p, color.red, color.green, color.blue, cover);

			x++;
			p += 4;
		} while(--len);
	}
}

// blend_solid_hspan_max
void
blend_solid_hspan_max(int x, int y, unsigned len, 
					  const color_type& c, const uint8* covers,
					  agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row(y) + (x << 2);
	do {
		rgb_color color = pattern->R5ColorAt(x, y);
		if (*covers) {
			if (*covers == 255) {
				ASSIGN_MAX(p, color.red, color.green, color.blue);
			} else {
				BLEND_MAX(p, color.red, color.green, color.blue, *covers);
			}
		}
		covers++;
		p += 4;
		x++;
	} while(--len);
}



// blend_solid_vspan_max
void
blend_solid_vspan_max(int x, int y, unsigned len, 
					  const color_type& c, const uint8* covers,
					  agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row(y) + (x << 2);
	do {
		rgb_color color = pattern->R5ColorAt(x, y);
		if (*covers) {
			if (*covers == 255) {
				ASSIGN_MAX(p, color.red, color.green, color.blue);
			} else {
				BLEND_MAX(p, color.red, color.green, color.blue, *covers);
			}
		}
		covers++;
		p += buffer->stride();
		y++;
	} while(--len);
}


// blend_color_hspan_max
void
blend_color_hspan_max(int x, int y, unsigned len, 
					  const color_type* colors, 
					  const uint8* covers, uint8 cover,
					  agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row(y) + (x << 2);
	if (covers) {
		// non-solid opacity
		do {
			if (*covers) {
				if (*covers == 255) {
					ASSIGN_MAX(p, colors->r, colors->g, colors->b);
				} else {
					BLEND_MAX(p, colors->r, colors->g, colors->b, *covers);
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
				ASSIGN_MAX(p, colors->r, colors->g, colors->b);
				p += 4;
				++colors;
			} while(--len);
		// solid partial opacity
		} else if (cover) {
			do {
				BLEND_MAX(p, colors->r, colors->g, colors->b, cover);
				p += 4;
				++colors;
			} while(--len);
		}
	}
}

#endif // DRAWING_MODE_MAX_H

