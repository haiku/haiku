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
	if (brightness_for((r), (g), (b)) \
		> brightness_for(_p.data8[2], _p.data8[1], _p.data8[0])) { \
		BLEND(d, (r), (g), (b), a); \
	} \
}

// ASSIGN_MAX
#define ASSIGN_MAX(d, r, g, b) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	if (brightness_for((r), (g), (b)) \
		> brightness_for(_p.data8[2], _p.data8[1], _p.data8[0])) { \
		d[0] = (b); \
		d[1] = (g); \
		d[2] = (r); \
		d[3] = 255; \
	} \
}


// blend_pixel_max
void
blend_pixel_max(int x, int y, const color_type& c, uint8 cover,
				agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color color = pattern->ColorAt(x, y);
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
	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (cover == 255) {
		do {
			rgb_color color = pattern->ColorAt(x, y);

			ASSIGN_MAX(p, color.red, color.green, color.blue);

			p += 4;
			x++;
		} while(--len);
	} else {
		do {
			rgb_color color = pattern->ColorAt(x, y);

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
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
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
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
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
	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (covers) {
		// non-solid opacity
		do {
			if (*covers && colors->a > 0) {
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
				if (colors->a > 0) {
					ASSIGN_MAX(p, colors->r, colors->g, colors->b);
				}
				p += 4;
				++colors;
			} while(--len);
		// solid partial opacity
		} else if (cover) {
			do {
				if (colors->a > 0) {
					BLEND_MAX(p, colors->r, colors->g, colors->b, cover);
				}
				p += 4;
				++colors;
			} while(--len);
		}
	}
}

#endif // DRAWING_MODE_MAX_H

