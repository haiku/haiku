// DrawingModeAlphaCC.h

#ifndef DRAWING_MODE_ALPHA_CC_H
#define DRAWING_MODE_ALPHA_CC_H

#include "DrawingMode.h"

// blend_alpha_cc
inline void
blend_alpha_cc(uint8* d1, uint8* d2, uint8* d3, uint8* da,
			   uint8 s1, uint8 s2, uint8 s3, uint16 a)
{
	blend_composite(d1, d2, d3, da, s1, s2, s3, a);
}

// assign_alpha_cc
inline void
assign_alpha_cc(uint8* d1, uint8* d2, uint8* d3, uint8* da,
				uint8 s1, uint8 s2, uint8 s3)
{
	*d1 = s1;
	*d2 = s2;
	*d3 = s3;
	*da = 255;
}


namespace agg
{
	template<class Order>
	class DrawingModeAlphaCC : public DrawingMode
	{
	public:
		typedef Order order_type;

		// constructor
		DrawingModeAlphaCC()
			: DrawingMode()
		{
		}

		// blend_pixel
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			uint16 alpha = fPatternHandler->HighColor().GetColor32().alpha * cover;
			if (alpha == 255*255) {
				assign_alpha_cc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								color.red, color.green, color.blue);
			} else {
				blend_alpha_cc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
							   color.red, color.green, color.blue, alpha);
			}
		}

		// blend_hline
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
			rgb_color color = fPatternHandler->HighColor().GetColor32();
			uint16 alpha = color.alpha * cover;
			if (alpha == 255*255) {
				// cache the low and high color as 32bit values
				// high color
				int32u vh;
				int8u* p8 = (int8u*)&vh;
				p8[Order::R] = color.red;
				p8[Order::G] = color.green;
				p8[Order::B] = color.blue;
				p8[Order::A] = 255;
				// low color
				color = fPatternHandler->LowColor().GetColor32();
				int32u vl;
				p8 = (int8u*)&vl;
				p8[Order::R] = color.red;
				p8[Order::G] = color.green;
				p8[Order::B] = color.blue;
				p8[Order::A] = 255;
				// row offset as 32bit pointer
				int32u* p32 = (int32u*)(m_rbuf->row(y)) + x;
				do {
					if (fPatternHandler->IsHighColor(x, y))
						*p32 = vh;
					else
						*p32 = vl;
					p32++;
					x++;
				} while(--len);
			} else {
				int8u* p = m_rbuf->row(y) + (x << 2);
				do {
					rgb_color color = fPatternHandler->R5ColorAt(x, y);
					blend_alpha_cc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								   color.red, color.green, color.blue, alpha);
					x++;
					p += 4;
				} while(--len);
			}
		}

		// blend_vline
		virtual	void blend_vline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
printf("DrawingModeAlphaCC::blend_vline()\n");
		}

		// blend_solid_hspan
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			uint8 hAlpha = fPatternHandler->HighColor().GetColor32().alpha;
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
				uint16 alpha = hAlpha * *covers;
				if (alpha) {
					if(alpha == 255*255) {
						assign_alpha_cc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										color.red, color.green, color.blue);
					} else {
						blend_alpha_cc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			uint8 hAlpha = fPatternHandler->HighColor().GetColor32().alpha;
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
				uint16 alpha = hAlpha * *covers;
				if (alpha) {
					if (alpha == 255*255) {
						assign_alpha_cc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									color.red, color.green, color.blue);
					} else {
						blend_alpha_cc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								   color.red, color.green, color.blue, alpha);
					}
				}
				covers++;
				p += m_rbuf->stride();
				y++;
			} while(--len);
		}


		// blend_color_hspan
		virtual	void blend_color_hspan(int x, int y, unsigned len, 
									   const color_type* colors, 
									   const int8u* covers,
									   int8u cover)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			uint8 hAlpha = fPatternHandler->HighColor().GetColor32().alpha;
			if (covers) {
				// non-solid opacity
				do {
					uint16 alpha = hAlpha * *covers;
					if (alpha) {
						if (alpha == 255*255) {
							assign_alpha_cc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										colors->r, colors->g, colors->b);
						} else {
							blend_alpha_cc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
						assign_alpha_cc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									colors->r, colors->g, colors->b);
						p += 4;
						++colors;
					} while(--len);
				// solid partial opacity
				} else if (alpha) {
					do {
						blend_alpha_cc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
									   const int8u* covers,
									   int8u cover)
		{
printf("DrawingModeAlphaCC::blend_color_vspan()\n");
		}

	};

	typedef DrawingModeAlphaCC<order_rgba32> DrawingModeRGBA32AlphaCC; //----DrawingModeRGBA32AlphaCC
	typedef DrawingModeAlphaCC<order_argb32> DrawingModeARGB32AlphaCC; //----DrawingModeARGB32AlphaCC
	typedef DrawingModeAlphaCC<order_abgr32> DrawingModeABGR32AlphaCC; //----DrawingModeABGR32AlphaCC
	typedef DrawingModeAlphaCC<order_bgra32> DrawingModeBGRA32AlphaCC; //----DrawingModeBGRA32AlphaCC
}

#endif // DRAWING_MODE_ALPHA_CC_H

