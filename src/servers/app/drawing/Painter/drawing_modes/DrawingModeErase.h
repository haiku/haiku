/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_ERASE on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_ERASE_H
#define DRAWING_MODE_ERASE_H

#include "DrawingMode.h"

// BLEND_ERASE
#define BLEND_ERASE(d, r, g, b, a) \
{ \
	BLEND(d, r, g, b, a); \
}

// ASSIGN_ERASE
#define ASSIGN_ERASE(d, r, g, b) \
{ \
	d[0] = (b); \
	d[1] = (g); \
	d[2] = (r); \
}

// blend_pixel_erase
void
blend_pixel_erase(int x, int y, const color_type& c, uint8 cover,
				  agg_buffer* buffer, const PatternHandler* pattern)
{
	if (pattern->IsHighColor(x, y)) {
		uint8* p = buffer->row(y) + (x << 2);
		rgb_color color = pattern->LowColor().GetColor32();
		if (cover == 255) {
			ASSIGN_ERASE(p, color.red, color.green, color.blue);
		} else {
			BLEND_ERASE(p, color.red, color.green, color.blue, cover);
		}
	}
}

// blend_hline_erase
void
blend_hline_erase(int x, int y, unsigned len, 
						 const color_type& c, uint8 cover,
						 agg_buffer* buffer, const PatternHandler* pattern)
{
	if (cover == 255) {
		rgb_color color = pattern->LowColor().GetColor32();
		uint32 v;
		uint8* p8 = (uint8*)&v;
		p8[0] = (uint8)color.blue;
		p8[1] = (uint8)color.green;
		p8[2] = (uint8)color.red;
		p8[3] = 255;
		uint32* p32 = (uint32*)(buffer->row(y)) + x;
		do {
			if (pattern->IsHighColor(x, y))
				*p32 = v;
			p32++;
			x++;
		} while(--len);
	} else {
		uint8* p = buffer->row(y) + (x << 2);
		rgb_color color = pattern->LowColor().GetColor32();
		do {
			if (pattern->IsHighColor(x, y)) {
				BLEND_ERASE(p, color.red, color.green, color.blue, cover);
			}
			x++;
			p += 4;
		} while(--len);
	}
}

// blend_solid_hspan_erase
void
blend_solid_hspan_erase(int x, int y, unsigned len, 
						const color_type& c, const uint8* covers,
						agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row(y) + (x << 2);
	rgb_color color = pattern->LowColor().GetColor32();
	do {
		if (pattern->IsHighColor(x, y)) {
			if (*covers) {
				if(*covers == 255) {
					ASSIGN_ERASE(p, color.red, color.green, color.blue);
				} else {
					BLEND_ERASE(p, color.red, color.green, color.blue, *covers);
				}
			}
		}
		covers++;
		p += 4;
		x++;
	} while(--len);
}



// blend_solid_vspan_erase
void
blend_solid_vspan_erase(int x, int y, unsigned len, 
						const color_type& c, const uint8* covers,
						agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row(y) + (x << 2);
	rgb_color color = pattern->LowColor().GetColor32();
	do {
		if (pattern->IsHighColor(x, y)) {
			if (*covers) {
				if (*covers == 255) {
					ASSIGN_ERASE(p, color.red, color.green, color.blue);
				} else {
					BLEND_ERASE(p, color.red, color.green, color.blue, *covers);
				}
			}
		}
		covers++;
		p += buffer->stride();
		y++;
	} while(--len);
}


// blend_color_hspan_erase
void
blend_color_hspan_erase(int x, int y, unsigned len, 
						const color_type* colors, 
						const uint8* covers, uint8 cover,
						agg_buffer* buffer, const PatternHandler* pattern)
{
	// TODO: compare this with BView
	uint8* p = buffer->row(y) + (x << 2);
	if (covers) {
		// non-solid opacity
		do {
			if(*covers) {
				if(*covers == 255) {
					ASSIGN_ERASE(p, colors->r, colors->g, colors->b);
				} else {
					BLEND_ERASE(p, colors->r, colors->g, colors->b, *covers);
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
				ASSIGN_ERASE(p, colors->r, colors->g, colors->b);
				p += 4;
				++colors;
			} while(--len);
		// solid partial opacity
		} else if (cover) {
			do {
				BLEND_ERASE(p, colors->r, colors->g, colors->b, cover);
				p += 4;
				++colors;
			} while(--len);
		}
	}
}

#endif // DRAWING_MODE_ERASE_H

