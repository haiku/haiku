/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_SELECT on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_SELECT_H
#define DRAWING_MODE_SELECT_H

#include "DrawingMode.h"

// BLEND_SELECT
#define BLEND_SELECT(d, r, g, b, a) \
{ \
	BLEND(d, r, g, b, a); \
}

// ASSIGN_SELECT
#define ASSIGN_SELECT(d, r, g, b) \
{ \
	d[0] = (b); \
	d[1] = (g); \
	d[2] = (r); \
	d[3] = 255; \
}

// compare
inline bool
compare(uint8* p, const rgb_color& high, const rgb_color& low, rgb_color* result)
{
	pixel32 _p;
	_p.data32 = *(uint32*)p;
	if (_p.data8[2] == high.red &&
		_p.data8[1] == high.green &&
		_p.data8[0] == high.blue) {
		result->red = low.red;
		result->green = low.green;
		result->blue = low.blue;
		return true;
	} else if (_p.data8[2] == low.red &&
			   _p.data8[1] == low.green &&
			   _p.data8[0] == low.blue) {
		result->red = high.red;
		result->green = high.green;
		result->blue = high.blue;
		return true;
	}
	return false;
}

// blend_pixel_select
void
blend_pixel_select(int x, int y, const color_type& c, uint8 cover,
				   agg_buffer* buffer, const PatternHandler* pattern)
{
	if (pattern->IsHighColor(x, y)) {
		uint8* p = buffer->row_ptr(y) + (x << 2);
		rgb_color high = pattern->HighColor();
		rgb_color low = pattern->LowColor();
		rgb_color color; 
		if (compare(p, high, low, &color)) {
			if (cover == 255) {
				ASSIGN_SELECT(p, color.red, color.green, color.blue);
			} else {
				BLEND_SELECT(p, color.red, color.green, color.blue, cover);
			}
		}
	}
}

// blend_hline_select
void
blend_hline_select(int x, int y, unsigned len, 
				   const color_type& c, uint8 cover,
				   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color high = pattern->HighColor();
	rgb_color low = pattern->LowColor();
	rgb_color color; 
	if (cover == 255) {
		do {
			if (pattern->IsHighColor(x, y)
				&& compare(p, high, low, &color))
				ASSIGN_SELECT(p, color.red, color.green, color.blue);
			x++;
			p += 4;
		} while(--len);
	} else {
		do {
			if (pattern->IsHighColor(x, y)
				&& compare(p, high, low, &color)) {
				BLEND_SELECT(p, color.red, color.green, color.blue, cover);
			}
			x++;
			p += 4;
		} while(--len);
	}
}

// blend_solid_hspan_select
void
blend_solid_hspan_select(int x, int y, unsigned len, 
						 const color_type& c, const uint8* covers,
						 agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color high = pattern->HighColor();
	rgb_color low = pattern->LowColor();
	rgb_color color; 
	do {
		if (pattern->IsHighColor(x, y)) {
			if (*covers && compare(p, high, low, &color)) {
				if (*covers == 255) {
					ASSIGN_SELECT(p, color.red, color.green, color.blue);
				} else {
					BLEND_SELECT(p, color.red, color.green, color.blue, *covers);
				}
			}
		}
		covers++;
		p += 4;
		x++;
	} while(--len);
}



// blend_solid_vspan_select
void
blend_solid_vspan_select(int x, int y, unsigned len, 
						 const color_type& c, const uint8* covers,
						 agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color high = pattern->HighColor();
	rgb_color low = pattern->LowColor();
	rgb_color color; 
	do {
		if (pattern->IsHighColor(x, y)) {
			if (*covers && compare(p, high, low, &color)) {
				if (*covers == 255) {
					ASSIGN_SELECT(p, color.red, color.green, color.blue);
				} else {
					BLEND_SELECT(p, color.red, color.green, color.blue, *covers);
				}
			}
		}
		covers++;
		p += buffer->stride();
		y++;
	} while(--len);
}


// blend_color_hspan_select
void
blend_color_hspan_select(int x, int y, unsigned len, 
						 const color_type* colors, 
						 const uint8* covers, uint8 cover,
						 agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color high = pattern->HighColor();
	rgb_color low = pattern->LowColor();
	rgb_color color; 
	if (covers) {
		// non-solid opacity
		do {
			if (*covers && colors->a > 0 && compare(p, high, low, &color)) {
				if (*covers == 255) {
					ASSIGN_SELECT(p, color.red, color.green, color.blue);
				} else {
					BLEND_SELECT(p, color.red, color.green, color.blue, *covers);
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
				if (colors->a > 0 && compare(p, high, low, &color)) {
					ASSIGN_SELECT(p, color.red, color.green, color.blue);
				}
				p += 4;
				++colors;
			} while(--len);
		// solid partial opacity
		} else if (cover) {
			do {
				if (colors->a > 0 && compare(p, high, low, &color)) {
					BLEND_SELECT(p, color.red, color.green, color.blue, cover);
				}
				p += 4;
				++colors;
			} while(--len);
		}
	}
}

#endif // DRAWING_MODE_SELECT_H

