/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_COPY on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_COPY_H
#define DRAWING_MODE_COPY_H

#include "DrawingMode.h"

// BLEND_COPY
#define BLEND_COPY(d1, d2, d3, da, s1, s2, s3, a, l1, l2, l3) \
{ \
	BLEND_FROM(d1, d2, d3, da, l1, l2, l3, s1, s2, s3, a); \
}

// ASSIGN_COPY
#define ASSIGN_COPY(d1, d2, d3, da, s1, s2, s3) \
{ \
	(d1) = (s1); \
	(d2) = (s2); \
	(d3) = (s3); \
	(da) = 255; \
}


template<class Order>
class DrawingModeCopy : public DrawingMode {
 public:
	typedef Order order_type;

	// constructor
	DrawingModeCopy()
		: DrawingMode()
	{
	}

	// blend_pixel
	virtual	void blend_pixel(int x, int y, const color_type& c, uint8 cover)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color color = fPatternHandler->R5ColorAt(x, y);
		if (cover == 255) {
			ASSIGN_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
						color.red, color.green, color.blue);
		} else {
			rgb_color l = fPatternHandler->LowColor().GetColor32();
			BLEND_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
					   color.red, color.green, color.blue, cover,
					   l.red, l.green, l.blue);
		}
	}

	// blend_hline
	virtual	void blend_hline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
		if (cover == 255) {
			// cache the low and high color as 32bit values
			// high color
			rgb_color color = fPatternHandler->HighColor().GetColor32();
			uint32 vh;
			uint8* p8 = (uint8*)&vh;
			p8[Order::R] = (uint8)color.red;
			p8[Order::G] = (uint8)color.green;
			p8[Order::B] = (uint8)color.blue;
			p8[Order::A] = 255;
			// low color
			color = fPatternHandler->LowColor().GetColor32();
			uint32 vl;
			p8 = (uint8*)&vl;
			p8[Order::R] = (uint8)color.red;
			p8[Order::G] = (uint8)color.green;
			p8[Order::B] = (uint8)color.blue;
			p8[Order::A] = 255;
			// row offset as 32bit pointer
			uint32* p32 = (uint32*)(fBuffer->row(y)) + x;
			do {
				if (fPatternHandler->IsHighColor(x, y))
					*p32 = vh;
				else
					*p32 = vl;
				p32++;
				x++;
			} while(--len);
		} else {
			uint8* p = fBuffer->row(y) + (x << 2);
			rgb_color l = fPatternHandler->LowColor().GetColor32();
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
				BLEND_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
						   color.red, color.green, color.blue, cover,
						   l.red, l.green, l.blue);
				x++;
				p += 4;
			} while(--len);
		}
	}

	// blend_vline
	virtual	void blend_vline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
printf("DrawingModeCopy::blend_vline()\n");
	}

	// blend_solid_hspan
	virtual	void blend_solid_hspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color l = fPatternHandler->LowColor().GetColor32();
		do {
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			if (*covers) {
				if(*covers == 255) {
					ASSIGN_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								color.red, color.green, color.blue);
				} else {
					BLEND_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							   color.red, color.green, color.blue, *covers,
							   l.red, l.green, l.blue);
				}
			}
			covers++;
			p += 4;
			x++;
		} while(--len);
	}



	// blend_solid_vspan
	virtual	void blend_solid_vspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color l = fPatternHandler->LowColor().GetColor32();
		do {
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			if (*covers) {
				if (*covers == 255) {
					ASSIGN_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								color.red, color.green, color.blue);
				} else {
					BLEND_COPY(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							   color.red, color.green, color.blue, *covers,
							   l.red, l.green, l.blue);
				}
			}
			covers++;
			p += fBuffer->stride();
			y++;
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
printf("DrawingModeCopy::blend_color_vspan()\n");
	}

};

typedef DrawingModeCopy<agg::order_rgba32> DrawingModeRGBA32Copy;
typedef DrawingModeCopy<agg::order_argb32> DrawingModeARGB32Copy;
typedef DrawingModeCopy<agg::order_abgr32> DrawingModeABGR32Copy;
typedef DrawingModeCopy<agg::order_bgra32> DrawingModeBGRA32Copy;

#endif // DRAWING_MODE_COPY_H

