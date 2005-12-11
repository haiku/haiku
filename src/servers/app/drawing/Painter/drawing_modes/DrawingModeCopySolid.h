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

template<class Order>
class DrawingModeCopySolid : public DrawingMode {
 public:
	// constructor
	DrawingModeCopySolid()
		: DrawingMode()
	{
	}

	// blend_pixel
	virtual	void blend_pixel(int x, int y, const color_type& c, uint8 cover)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		if (cover == 255) {
			ASSIGN_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
						c.r, c.g, c.b);
		} else {
			rgb_color l = fPatternHandler->LowColor().GetColor32();
			BLEND_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
					   c.r, c.g, c.b, cover,
					   l.red, l.green, l.blue);
		}
	}

	// blend_hline
	virtual	void blend_hline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
		if(cover == 255) {
			// cache the color as 32bit value
			uint32 v;
			uint8* p8 = (uint8*)&v;
			p8[Order::R] = (uint8)c.r;
			p8[Order::G] = (uint8)c.g;
			p8[Order::B] = (uint8)c.b;
			p8[Order::A] = 255;
			// row offset as 32bit pointer
			uint32* p32 = (uint32*)(fBuffer->row(y)) + x;
			do {
				*p32 = v;
				p32++;
			} while(--len);
		} else {
			uint8* p = fBuffer->row(y) + (x << 2);
			rgb_color l = fPatternHandler->LowColor().GetColor32();
			do {
				BLEND_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
						   c.r, c.g, c.b, cover,
						   l.red, l.green, l.blue);
				p += 4;
			} while(--len);
		}
	}

	// blend_vline
	virtual	void blend_vline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
printf("DrawingModeCopySolid::blend_vline()\n");
	}

	// blend_solid_hspan
	virtual	void blend_solid_hspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color l = fPatternHandler->LowColor().GetColor32();
		do {
			if (*covers) {
				if(*covers == 255) {
					ASSIGN_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								c.r, c.g, c.b);
				} else {
					BLEND_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							   c.r, c.g, c.b, *covers,
							   l.red, l.green, l.blue);
				}
			}
			covers++;
			p += 4;
		} while(--len);
	}



	// blend_solid_vspan
	virtual	void blend_solid_vspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color l = fPatternHandler->LowColor().GetColor32();
		do {
			if (*covers) {
				if (*covers == 255) {
					ASSIGN_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								c.r, c.g, c.b);
				} else {
					BLEND_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							   c.r, c.g, c.b, *covers,
							   l.red, l.green, l.blue);
				}
			}
			covers++;
			p += fBuffer->stride();
		} while(--len);
	}


	// blend_color_hspan
	virtual	void blend_color_hspan(int x, int y, unsigned len, 
								   const color_type* colors, 
								   const uint8* covers,
								   uint8 cover)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color l = fPatternHandler->LowColor().GetColor32();
		if (covers) {
			// non-solid opacity
			do {
				if(*covers) {
					if(*covers == 255) {
						ASSIGN_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									colors->r, colors->g, colors->b);
					} else {
						BLEND_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								   colors->r, colors->g, colors->b, *covers,
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
					ASSIGN_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								colors->r, colors->g, colors->b);
					p += 4;
					++colors;
				} while(--len);
			// solid partial opacity
			} else if (cover) {
				do {
					BLEND_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							   colors->r, colors->g, colors->b, cover,
							   l.red, l.green, l.blue);
					p += 4;
					++colors;
				} while(--len);
			}
		}
	}


	// blend_color_vspan
	virtual	void blend_color_vspan(int x, int y, unsigned len, 
								   const color_type* colors, 
								   const uint8* covers,
								   uint8 cover)
	{
printf("DrawingModeCopySolid::blend_color_vspan()\n");
	}

};

typedef DrawingModeCopySolid<agg::order_rgba32> DrawingModeRGBA32CopySolid;
typedef DrawingModeCopySolid<agg::order_argb32> DrawingModeARGB32CopySolid;
typedef DrawingModeCopySolid<agg::order_abgr32> DrawingModeABGR32CopySolid;
typedef DrawingModeCopySolid<agg::order_bgra32> DrawingModeBGRA32CopySolid;

#endif // DRAWING_MODE_COPY_SOLID_H

