// DrawingModeAlphaPC.h

#ifndef DRAWING_MODE_ALPHA_PC_H
#define DRAWING_MODE_ALPHA_PC_H

#include "DrawingMode.h"

// BLEND_ALPHA_PC
#define BLEND_ALPHA_PC(d1, d2, d3, da, s1, s2, s3, a) \
{ \
	BLEND_COMPOSITE16(d1, d2, d3, da, s1, s2, s3, a); \
}

// ASSIGN_ALPHA_PC
#define ASSIGN_ALPHA_PC(d1, d2, d3, da, s1, s2, s3) \
{ \
	(d1) = (s1); \
	(d2) = (s2); \
	(d3) = (s3); \
	(da) = 255; \
}


template<class Order>
class DrawingModeAlphaPC : public DrawingMode {
 public:
	// constructor
	DrawingModeAlphaPC()
		: DrawingMode()
	{
	}

	// blend_pixel
	virtual	void blend_pixel(int x, int y, const color_type& c, uint8 cover)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color color = fPatternHandler->R5ColorAt(x, y);
		uint16 alpha = color.alpha * cover;
		if (alpha == 255*255) {
			ASSIGN_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							color.red, color.green, color.blue);
		} else {
			BLEND_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
						   color.red, color.green, color.blue, alpha);
		}
	}

	// blend_hline
	virtual	void blend_hline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		do {
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			uint16 alpha = color.alpha * cover;
			if (alpha) {
				if (alpha == 255) {
					ASSIGN_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									color.red, color.green, color.blue);
				} else {
					BLEND_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								   color.red, color.green, color.blue, alpha);
				}
			}
			x++;
			p += 4;
		} while(--len);
	}

	// blend_vline
	virtual	void blend_vline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
printf("DrawingModeAlphaPC::blend_vline()\n");
	}

	// blend_solid_hspan
	virtual	void blend_solid_hspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		do {
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			uint16 alpha = color.alpha * *covers;
			if (alpha) {
				if(alpha == 255*255) {
					ASSIGN_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									color.red, color.green, color.blue);
				} else {
					BLEND_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
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
		do {
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			uint16 alpha = color.alpha * *covers;
			if (alpha) {
				if (alpha == 255*255) {
					ASSIGN_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									color.red, color.green, color.blue);
				} else {
					BLEND_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
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
		if (covers) {
			// non-solid opacity
			do {
				uint16 alpha = colors->a * *covers;
				if (alpha) {
					if (alpha == 255*255) {
						ASSIGN_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
										colors->r, colors->g, colors->b);
					} else {
						BLEND_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									   colors->r, colors->g, colors->b, alpha);
					}
				}
				covers++;
				p += 4;
				++colors;
			} while(--len);
		} else {
			// solid full opcacity
			uint16 alpha = colors->a * cover;
			if (alpha == 255*255) {
				do {
					ASSIGN_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									colors->r, colors->g, colors->b);
					p += 4;
					++colors;
				} while(--len);
			// solid partial opacity
			} else if (alpha) {
				do {
					BLEND_ALPHA_PC(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
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
printf("DrawingModeAlphaPC::blend_color_vspan()\n");
	}

};

typedef DrawingModeAlphaPC<agg::order_rgba32> DrawingModeRGBA32AlphaPC;
typedef DrawingModeAlphaPC<agg::order_argb32> DrawingModeARGB32AlphaPC;
typedef DrawingModeAlphaPC<agg::order_abgr32> DrawingModeABGR32AlphaPC;
typedef DrawingModeAlphaPC<agg::order_bgra32> DrawingModeBGRA32AlphaPC;

#endif // DRAWING_MODE_ALPHA_PC_H

