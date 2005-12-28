/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_COPY ignoring the pattern (solid) on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_COPY_SOLID_H
#define DRAWING_MODE_COPY_SOLID_H

#include "DrawingModeCopy.h"

// blend_pixel_copy_solid
void
blend_pixel_copy_solid(int x, int y, const color_type& c, uint8 cover,
					   agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row(y) + (x << 2);
	if (cover == 255) {
		ASSIGN_COPY(p, c.r, c.g, c.b, c.a);
	} else {
		rgb_color l = pattern->LowColor().GetColor32();
		BLEND_COPY(p, c.r, c.g, c.b, cover,
				   l.red, l.green, l.blue);
	}
}

// blend_hline_copy_solid
void
blend_hline_copy_solid(int x, int y, unsigned len, 
					   const color_type& c, uint8 cover,
					   agg_buffer* buffer, const PatternHandler* pattern)
{
	if (cover == 255) {
		// cache the color as 32bit value
		uint32 v;
		uint8* p8 = (uint8*)&v;
		p8[0] = (uint8)c.b;
		p8[1] = (uint8)c.g;
		p8[2] = (uint8)c.r;
		p8[3] = 255;
		// row offset as 32bit pointer
		uint32* p32 = (uint32*)(buffer->row(y)) + x;
		do {
			*p32 = v;
			p32++;
		} while(--len);
	} else {
		uint8* p = buffer->row(y) + (x << 2);
		rgb_color l = pattern->LowColor().GetColor32();
		do {
			BLEND_COPY(p, c.r, c.g, c.b, cover,
					   l.red, l.green, l.blue);
			p += 4;
		} while(--len);
	}
}

// blend_solid_hspan_copy_solid
void
blend_solid_hspan_copy_solid(int x, int y, unsigned len, 
							 const color_type& c, const uint8* covers,
							 agg_buffer* buffer,
							 const PatternHandler* pattern)
{
	uint8* p = buffer->row(y) + (x << 2);
	rgb_color l = pattern->LowColor().GetColor32();
	do {
		if (*covers) {
			if(*covers == 255) {
				ASSIGN_COPY(p, c.r, c.g, c.b, c.a);
			} else {
				BLEND_COPY(p, c.r, c.g, c.b, *covers,
						   l.red, l.green, l.blue);
			}
		}
		covers++;
		p += 4;
	} while(--len);
}



// blend_solid_vspan_copy_solid
void
blend_solid_vspan_copy_solid(int x, int y, unsigned len, 
							 const color_type& c, const uint8* covers,
							 agg_buffer* buffer,
							 const PatternHandler* pattern)
{
	uint8* p = buffer->row(y) + (x << 2);
	rgb_color l = pattern->LowColor().GetColor32();
	do {
		if (*covers) {
			if (*covers == 255) {
				ASSIGN_COPY(p, c.r, c.g, c.b, c.a);
			} else {
				BLEND_COPY(p, c.r, c.g, c.b, *covers,
						   l.red, l.green, l.blue);
			}
		}
		covers++;
		p += buffer->stride();
	} while(--len);
}


// blend_color_hspan_copy_solid
void
blend_color_hspan_copy_solid(int x, int y, unsigned len, 
							 const color_type* colors, const uint8* covers,
							 uint8 cover,
							 agg_buffer* buffer,
							 const PatternHandler* pattern)
{
	uint8* p = buffer->row(y) + (x << 2);
	rgb_color l = pattern->LowColor().GetColor32();
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

#endif // DRAWING_MODE_COPY_SOLID_H

