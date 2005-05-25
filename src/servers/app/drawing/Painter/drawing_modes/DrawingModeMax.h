/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_MAX on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_MAX_H
#define DRAWING_MODE_MAX_H

#include <SupportDefs.h>

#include "DrawingMode.h"
#include "PatternHandler.h"

// BLEND_MAX
#define BLEND_MAX(d1, d2, d3, da, s1, s2, s3, a) \
{ \
	uint8 t1 = max_c((d1), (s1)); \
	uint8 t2 = max_c((d2), (s2)); \
	uint8 t3 = max_c((d3), (s3)); \
	BLEND(d1, d2, d3, da, t1, t2, t3, a); \
}

// ASSIGN_MAX
#define ASSIGN_MAX(d1, d2, d3, da, s1, s2, s3) \
{ \
	(d1) = max_c((d1), (s1)); \
	(d2) = max_c((d2), (s2)); \
	(d3) = max_c((d3), (s3)); \
	(da) = 255; \
}


template<class Order>
class DrawingModeMax : public DrawingMode {
 public:
	// constructor
	DrawingModeMax()
		: DrawingMode()
	{
	}

	// blend_pixel
	virtual	void blend_pixel(int x, int y, const color_type& c, uint8 cover)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color color = fPatternHandler->R5ColorAt(x, y);
		if (cover == 255) {
			ASSIGN_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
					   color.red, color.green, color.blue);
		} else {
			BLEND_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
					  color.red, color.green, color.blue, cover);
		}
	}

	// blend_hline
	virtual	void blend_hline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		if (cover == 255) {
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);

				ASSIGN_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
						   color.red, color.green, color.blue);

				p += 4;
				x++;
			} while(--len);
		} else {
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);

				BLEND_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
						  color.red, color.green, color.blue, cover);

				x++;
				p += 4;
			} while(--len);
		}
	}

	// blend_vline
	virtual	void blend_vline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
printf("DrawingModeMax::blend_vline()\n");
	}

	//--------------------------------------------------------------------
	virtual	void blend_solid_hspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		do {
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			if (*covers) {
				if (*covers == 255) {
					ASSIGN_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							   color.red, color.green, color.blue);
				} else {
					BLEND_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							  color.red, color.green, color.blue, *covers);
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
		do {
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			if (*covers) {
				if (*covers == 255) {
					ASSIGN_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							   color.red, color.green, color.blue);
				} else {
					BLEND_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							  color.red, color.green, color.blue, *covers);
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
				if (*covers) {
					if (*covers == 255) {
						ASSIGN_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								   colors->r, colors->g, colors->b);
					} else {
						BLEND_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								  colors->r, colors->g, colors->b, *covers);
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
					ASSIGN_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							   colors->r, colors->g, colors->b);
					p += 4;
					++colors;
				} while(--len);
			// solid partial opacity
			} else if (cover) {
				do {
					BLEND_MAX(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							  colors->r, colors->g, colors->b, cover);
					p += 4;
					++colors;
				} while(--len);
			}
		}
	}


	//--------------------------------------------------------------------
	virtual	void blend_color_vspan(int x, int y, unsigned len, 
								   const color_type* colors, 
								   const uint8* covers,
								   uint8 cover)
	{
printf("DrawingModeMax::blend_color_vspan()\n");
	}

};

typedef DrawingModeMax<agg::order_rgba32> DrawingModeRGBA32Max;
typedef DrawingModeMax<agg::order_argb32> DrawingModeARGB32Max;
typedef DrawingModeMax<agg::order_abgr32> DrawingModeABGR32Max;
typedef DrawingModeMax<agg::order_bgra32> DrawingModeBGRA32Max;

#endif // DRAWING_MODE_MAX_H

