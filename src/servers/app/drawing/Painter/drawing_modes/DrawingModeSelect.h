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
#define BLEND_SELECT(d1, d2, d3, da, s1, s2, s3, a) \
{ \
	BLEND(d1, d2, d3, da, s1, s2, s3, a); \
}

// ASSIGN_SELECT
#define ASSIGN_SELECT(d1, d2, d3, da, s1, s2, s3) \
{ \
	(d1) = (s1); \
	(d2) = (s2); \
	(d3) = (s3); \
	(da) = 255; \
}


template<class Order>
class DrawingModeSelect : public DrawingMode {
 public:
	// constructor
	DrawingModeSelect()
		: DrawingMode()
	{
	}

	// compare
	inline bool compare(uint8* p,
						const rgb_color& high,
						const rgb_color& low, rgb_color* result)
	{
		if (p[Order::R] == high.red &&
			p[Order::G] == high.green &&
			p[Order::B] == high.blue) {
			result->red = low.red;
			result->green = low.green;
			result->blue = low.blue;
			return true;
		} else if (p[Order::R] == low.red &&
				   p[Order::G] == low.green &&
				   p[Order::B] == low.blue) {
			result->red = high.red;
			result->green = high.green;
			result->blue = high.blue;
			return true;
		}
		return false;
	}


	// blend_pixel
	virtual	void blend_pixel(int x, int y, const color_type& c, uint8 cover)
	{
		if (fPatternHandler->IsHighColor(x, y)) {
			uint8* p = fBuffer->row(y) + (x << 2);
			rgb_color high = fPatternHandler->HighColor().GetColor32();
			rgb_color low = fPatternHandler->LowColor().GetColor32();
			rgb_color color; 
			if (compare(p, high, low, &color)) {
				if (cover == 255) {
					ASSIGN_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								  color.red, color.green, color.blue);
				} else {
					BLEND_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								 color.red, color.green, color.blue, cover);
				}
			}
		}
	}

	// blend_hline
	virtual	void blend_hline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color high = fPatternHandler->HighColor().GetColor32();
		rgb_color low = fPatternHandler->LowColor().GetColor32();
		rgb_color color; 
		if (cover == 255) {
			do {
				if (fPatternHandler->IsHighColor(x, y)
					&& compare(p, high, low, &color))
					ASSIGN_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								  color.red, color.green, color.blue);
				x++;
				p += 4;
			} while(--len);
		} else {
			do {
				if (fPatternHandler->IsHighColor(x, y)
					&& compare(p, high, low, &color)) {
					BLEND_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							   color.red, color.green, color.blue, cover);
				}
				x++;
				p += 4;
			} while(--len);
		}
	}

	// blend_vline
	virtual	void blend_vline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
printf("DrawingModeSelect::blend_vline()\n");
	}

	// blend_solid_hspan
	virtual	void blend_solid_hspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color high = fPatternHandler->HighColor().GetColor32();
		rgb_color low = fPatternHandler->LowColor().GetColor32();
		rgb_color color; 
		do {
			if (fPatternHandler->IsHighColor(x, y)) {
				if (*covers && compare(p, high, low, &color)) {
					if (*covers == 255) {
						ASSIGN_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									  color.red, color.green, color.blue);
					} else {
						BLEND_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									 color.red, color.green, color.blue, *covers);
					}
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
		rgb_color high = fPatternHandler->HighColor().GetColor32();
		rgb_color low = fPatternHandler->LowColor().GetColor32();
		rgb_color color; 
		do {
			if (fPatternHandler->IsHighColor(x, y)) {
				if (*covers && compare(p, high, low, &color)) {
					if (*covers == 255) {
						ASSIGN_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									  color.red, color.green, color.blue);
					} else {
						BLEND_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									 color.red, color.green, color.blue, *covers);
					}
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
		rgb_color high = fPatternHandler->HighColor().GetColor32();
		rgb_color low = fPatternHandler->LowColor().GetColor32();
		rgb_color color; 
		if (covers) {
			// non-solid opacity
			do {
				if (*covers && compare(p, high, low, &color)) {
					if (*covers == 255) {
						ASSIGN_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									  color.red, color.green, color.blue);
					} else {
						BLEND_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									 color.red, color.green, color.blue, *covers);
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
					if (compare(p, high, low, &color)) {
						ASSIGN_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									  color.red, color.green, color.blue);
					}
					p += 4;
					++colors;
				} while(--len);
			// solid partial opacity
			} else if (cover) {
				do {
					if (compare(p, high, low, &color)) {
						BLEND_SELECT(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									 color.red, color.green, color.blue, *covers);
					}
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
printf("DrawingModeSelect::blend_color_vspan()\n");
	}

};

typedef DrawingModeSelect<agg::order_rgba32> DrawingModeRGBA32Select;
typedef DrawingModeSelect<agg::order_argb32> DrawingModeARGB32Select;
typedef DrawingModeSelect<agg::order_abgr32> DrawingModeABGR32Select;
typedef DrawingModeSelect<agg::order_bgra32> DrawingModeBGRA32Select;

#endif // DRAWING_MODE_SELECT_H

