/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_OVER on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_OVER_SOLID_H
#define DRAWING_MODE_OVER_SOLID_H

#include "DrawingModeOver.h"

// blend_pixel_over_solid
void
blend_pixel_over_solid(int x, int y, const color_type& c, uint8 cover,
					   agg_buffer* buffer, const PatternHandler* pattern)
{
	if (pattern->IsSolidLow())
		return;

	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (cover == 255) {
		ASSIGN_OVER(p, c.r, c.g, c.b);
	} else {
		BLEND_OVER(p, c.r, c.g, c.b, cover);
	}
}

// blend_hline_over_solid
void
blend_hline_over_solid(int x, int y, unsigned len, 
					   const color_type& c, uint8 cover,
					   agg_buffer* buffer, const PatternHandler* pattern)
{
	if (pattern->IsSolidLow())
		return;

	if (cover == 255) {
		uint32 v;
		uint8* p8 = (uint8*)&v;
		p8[0] = (uint8)c.b;
		p8[1] = (uint8)c.g;
		p8[2] = (uint8)c.r;
		p8[3] = 255;
		uint32* p32 = (uint32*)(buffer->row_ptr(y)) + x;
		do {
			*p32 = v;
			p32++;
			x++;
		} while(--len);
	} else {
		uint8* p = buffer->row_ptr(y) + (x << 2);
		do {
			BLEND_OVER(p, c.r, c.g, c.b, cover);
			x++;
			p += 4;
		} while(--len);
	}
}

// blend_solid_hspan_over_solid
void
blend_solid_hspan_over_solid(int x, int y, unsigned len, 
							 const color_type& c, const uint8* covers,
							 agg_buffer* buffer, const PatternHandler* pattern)
{
	if (pattern->IsSolidLow())
		return;

	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		if (*covers) {
			if (*covers == 255) {
				ASSIGN_OVER(p, c.r, c.g, c.b);
			} else {
				BLEND_OVER(p, c.r, c.g, c.b, *covers);
			}
		}
		covers++;
		p += 4;
		x++;
	} while(--len);
}

// blend_solid_vspan_over_solid
void
blend_solid_vspan_over_solid(int x, int y, unsigned len, 
							 const color_type& c, const uint8* covers,
							 agg_buffer* buffer, const PatternHandler* pattern)
{
	if (pattern->IsSolidLow())
		return;

	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		if (*covers) {
			if (*covers == 255) {
				ASSIGN_OVER(p, c.r, c.g, c.b);
			} else {
				BLEND_OVER(p, c.r, c.g, c.b, *covers);
			}
		}
		covers++;
		p += buffer->stride();
		y++;
	} while(--len);
}

#endif // DRAWING_MODE_OVER_H

