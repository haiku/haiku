// DrawingModeAlphaCC.h

#ifndef DRAWING_MODE_ALPHA_CC_H
#define DRAWING_MODE_ALPHA_CC_H

#include "DrawingMode.h"

// BLEND_ALPHA_CC
#define BLEND_ALPHA_CC(d1, d2, d3, da, s1, s2, s3, a) \
{ \
	BLEND_COMPOSITE16(d1, d2, d3, da, s1, s2, s3, a); \
}

// ASSIGN_ALPHA_CC
#define ASSIGN_ALPHA_CC(d1, d2, d3, da, s1, s2, s3) \
{ \
	(d1) = (s1); \
	(d2) = (s2); \
	(d3) = (s3); \
	(da) = 255; \
}


template<class Order>
class DrawingModeAlphaCC : public DrawingMode {
 public:
	// constructor
	DrawingModeAlphaCC()
		: DrawingMode()
	{
	}

	// blend_pixel
	virtual	void blend_pixel(int x, int y, const color_type& c, uint8 cover)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color color = fPatternHandler->R5ColorAt(x, y);
		uint16 alpha = fPatternHandler->HighColor().GetColor32().alpha * cover;
		if (alpha == 255*255) {
			ASSIGN_ALPHA_CC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							color.red, color.green, color.blue);
		} else {
			BLEND_ALPHA_CC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
						   color.red, color.green, color.blue, alpha);
		}
	}

	// blend_hline
	virtual	void blend_hline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
		rgb_color color = fPatternHandler->HighColor().GetColor32();
		uint16 alpha = color.alpha * cover;
		if (alpha == 255*255) {
			// cache the low and high color as 32bit values
			// high color
			uint32 vh;
			uint8* p8 = (uint8*)&vh;
			p8[Order::R] = color.red;
			p8[Order::G] = color.green;
			p8[Order::B] = color.blue;
			p8[Order::A] = 255;
			// low color
			color = fPatternHandler->LowColor().GetColor32();
			uint32 vl;
			p8 = (uint8*)&vl;
			p8[Order::R] = color.red;
			p8[Order::G] = color.green;
			p8[Order::B] = color.blue;
			p8[Order::A] = 255;
			// row offset as 32 bit pointer
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
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
				BLEND_ALPHA_CC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							   color.red, color.green, color.blue, alpha);
				x++;
				p += 4;
			} while(--len);
		}
	}

	// blend_vline
	virtual	void blend_vline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
printf("DrawingModeAlphaCC::blend_vline()\n");
	}

	// blend_solid_hspan
	virtual	void blend_solid_hspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		uint8 hAlpha = fPatternHandler->HighColor().GetColor32().alpha;
		do {
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			uint16 alpha = hAlpha * *covers;
			if (alpha) {
				if (alpha == 255*255) {
					ASSIGN_ALPHA_CC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									color.red, color.green, color.blue);
				} else {
					BLEND_ALPHA_CC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								   color.red, color.green, color.blue, alpha);
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
		uint8 hAlpha = fPatternHandler->HighColor().GetColor32().alpha;
		do {
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			uint16 alpha = hAlpha * *covers;
			if (alpha) {
				if (alpha == 255*255) {
					ASSIGN_ALPHA_CC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									color.red, color.green, color.blue);
				} else {
					BLEND_ALPHA_CC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								   color.red, color.green, color.blue, alpha);
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
		uint8 hAlpha = fPatternHandler->HighColor().GetColor32().alpha;
		if (covers) {
			// non-solid opacity
			do {
				uint16 alpha = hAlpha * *covers;
				if (alpha) {
					if (alpha == 255*255) {
						ASSIGN_ALPHA_CC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
										colors->r, colors->g, colors->b);
					} else {
						BLEND_ALPHA_CC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									   colors->r, colors->g, colors->b, alpha);
					}
				}
				covers++;
				p += 4;
				++colors;
			} while(--len);
		} else {
			// solid full opcacity
			uint16 alpha = hAlpha * cover;
			if (alpha == 255*255) {
				do {
					ASSIGN_ALPHA_CC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									colors->r, colors->g, colors->b);
					p += 4;
					++colors;
				} while(--len);
			// solid partial opacity
			} else if (alpha) {
				do {
					BLEND_ALPHA_CC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								   colors->r, colors->g, colors->b, alpha);
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
printf("DrawingModeAlphaCC::blend_color_vspan()\n");
	}

};

typedef DrawingModeAlphaCC<agg::order_rgba32> DrawingModeRGBA32AlphaCC;
typedef DrawingModeAlphaCC<agg::order_argb32> DrawingModeARGB32AlphaCC;
typedef DrawingModeAlphaCC<agg::order_abgr32> DrawingModeABGR32AlphaCC;
typedef DrawingModeAlphaCC<agg::order_bgra32> DrawingModeBGRA32AlphaCC;

#endif // DRAWING_MODE_ALPHA_CC_H

