/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2015, Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_ALPHA in "Pixel Composite" mode on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_ALPHA_PC_SOLID_H
#define DRAWING_MODE_ALPHA_PC_SOLID_H

#include "DrawingMode.h"


#define BLEND_ALPHA_PC(d, r, g, b, a) \
{ \
	BLEND_COMPOSITE16(d, r, g, b, a); \
}


#define ASSIGN_ALPHA_PC(d, r, g, b) \
{ \
	d[0] = (b); \
	d[1] = (g); \
	d[2] = (r); \
	d[3] = 255; \
}


void
blend_pixel_alpha_pc_solid(int x, int y, const color_type& color, uint8 cover,
					 agg_buffer* buffer, const PatternHandler*)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	uint16 alpha = color.a * cover;
	if (alpha == 255 * 255) {
		ASSIGN_ALPHA_PC(p, color.r, color.g, color.b);
	} else {
		BLEND_ALPHA_PC(p, color.r, color.g, color.b, alpha);
	}
}


void
blend_hline_alpha_pc_solid(int x, int y, unsigned len,
					 const color_type& color, uint8 cover,
					 agg_buffer* buffer, const PatternHandler*)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	uint16 alpha = color.a * cover;
	if (alpha == 0)
		return;

	if (alpha == 255 * 255) {
		do {
			ASSIGN_ALPHA_PC(p, color.r, color.g, color.b);
			p += 4;
		} while(--len);
		return;
	}

	do {
		BLEND_ALPHA_PC(p, color.r, color.g, color.b, alpha);
		p += 4;
	} while(--len);
}


void
blend_solid_hspan_alpha_pc_solid(int x, int y, unsigned len,
						   const color_type& color, const uint8* covers,
						   agg_buffer* buffer, const PatternHandler*)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		uint16 alpha = color.a * *covers;
		if (alpha) {
			if(alpha == 255 * 255) {
				ASSIGN_ALPHA_PC(p, color.r, color.g, color.b);
			} else {
				BLEND_ALPHA_PC(p, color.r, color.g, color.b, alpha);
			}
		}
		covers++;
		p += 4;
	} while(--len);
}


void
blend_solid_vspan_alpha_pc_solid(int x, int y, unsigned len,
						   const color_type& color, const uint8* covers,
						   agg_buffer* buffer, const PatternHandler*)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		uint16 alpha = color.a * *covers;
		if (alpha) {
			if (alpha == 255 * 255) {
				ASSIGN_ALPHA_PC(p, color.r, color.g, color.b);
			} else {
				BLEND_ALPHA_PC(p, color.r, color.g, color.b, alpha);
			}
		}
		covers++;
		p += buffer->stride();
	} while(--len);
}


void
blend_color_hspan_alpha_pc_solid(int x, int y, unsigned len,
						   const color_type* colors,
						   const uint8* covers, uint8 cover,
						   agg_buffer* buffer, const PatternHandler*)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (covers) {
		// non-solid opacity
		do {
			uint16 alpha = colors->a * *covers;
			if (alpha) {
				if (alpha == 255 * 255) {
					ASSIGN_ALPHA_PC(p, colors->r, colors->g, colors->b);
				} else {
					BLEND_ALPHA_PC(p, colors->r, colors->g, colors->b, alpha);
				}
			}
			covers++;
			p += 4;
			++colors;
		} while(--len);
	} else {
		// solid full opcacity
		uint16 alpha = colors->a * cover;
		if (alpha == 255 * 255) {
			do {
				ASSIGN_ALPHA_PC(p, colors->r, colors->g, colors->b);
				p += 4;
				++colors;
			} while(--len);
		// solid partial opacity
		} else if (alpha) {
			do {
				BLEND_ALPHA_PC(p, colors->r, colors->g, colors->b, alpha);
				p += 4;
				++colors;
			} while(--len);
		}
	}
}


#endif // DRAWING_MODE_ALPHA_PC_SOLID_H
