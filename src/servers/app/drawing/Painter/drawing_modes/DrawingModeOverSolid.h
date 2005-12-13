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

template<class Order>
class DrawingModeOverSolid : public DrawingMode {
 public:
	// constructor
	DrawingModeOverSolid()
		: DrawingMode()
	{
	}

	// blend_pixel
	virtual	void blend_pixel(int x, int y, const color_type& c, uint8 cover)
	{
		if (fPatternHandler->IsSolidLow())
			return;

		uint8* p = fBuffer->row(y) + (x << 2);
		if (cover == 255) {
			ASSIGN_OVER(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
						c.r, c.g, c.b);
		} else {
			BLEND_OVER(p, c.r, c.g, c.b, cover);
		}
	}

	// blend_hline
	virtual	void blend_hline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
		if (fPatternHandler->IsSolidLow())
			return;

		if(cover == 255) {
			uint32 v;
			uint8* p8 = (uint8*)&v;
			p8[Order::R] = (uint8)c.r;
			p8[Order::G] = (uint8)c.g;
			p8[Order::B] = (uint8)c.b;
			p8[Order::A] = 255;
			uint32* p32 = (uint32*)(fBuffer->row(y)) + x;
			do {
				*p32 = v;
				p32++;
				x++;
			} while(--len);
		} else {
			uint8* p = fBuffer->row(y) + (x << 2);
			do {
				BLEND_OVER(p, c.r, c.g, c.b, cover);
				x++;
				p += 4;
			} while(--len);
		}
	}

	// blend_vline
	virtual	void blend_vline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
printf("DrawingModeOver::blend_vline()\n");
	}

	// blend_solid_hspan
	virtual	void blend_solid_hspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		if (fPatternHandler->IsSolidLow())
			return;

		uint8* p = fBuffer->row(y) + (x << 2);
		do {
			if (*covers) {
				if (*covers == 255) {
					ASSIGN_OVER(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								c.r, c.g, c.b);
				} else {
					BLEND_OVER(p, c.r, c.g, c.b, *covers);
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
		if (fPatternHandler->IsSolidLow())
			return;

		uint8* p = fBuffer->row(y) + (x << 2);
		do {
			if (*covers) {
				if (*covers == 255) {
					ASSIGN_OVER(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								c.r, c.g, c.b);
				} else {
					BLEND_OVER(p, c.r, c.g, c.b, *covers);
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
		if (covers) {
			// non-solid opacity
			do {
//				if (*covers) {
if (*covers && (colors->a & 0xff)) {
					if (*covers == 255) {
						ASSIGN_OVER(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									colors->r, colors->g, colors->b);
					} else {
						BLEND_OVER(p, colors->r, colors->g, colors->b, *covers);
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
					ASSIGN_OVER(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								colors->r, colors->g, colors->b);
}
					p += 4;
					++colors;
				} while(--len);
			// solid partial opacity
			} else if (cover) {
				do {
					BLEND_OVER(p, colors->r, colors->g, colors->b, cover);
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
printf("DrawingModeOver::blend_color_vspan()\n");
	}

};

typedef DrawingModeOverSolid<agg::order_rgba32> DrawingModeRGBA32OverSolid;
typedef DrawingModeOverSolid<agg::order_argb32> DrawingModeARGB32OverSolid;
typedef DrawingModeOverSolid<agg::order_abgr32> DrawingModeABGR32OverSolid;
typedef DrawingModeOverSolid<agg::order_bgra32> DrawingModeBGRA32OverSolid;

#endif // DRAWING_MODE_OVER_H

