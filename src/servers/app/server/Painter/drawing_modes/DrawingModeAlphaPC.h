// DrawingModeAlphaPC.h

#ifndef DRAWING_MODE_ALPHA_PC_H
#define DRAWING_MODE_ALPHA_PC_H

#include "DrawingMode.h"

// blend_alpha_pc
inline void
blend_alpha_pc(uint8* d1, uint8* d2, uint8* d3, uint8* da,
			   uint8 s1, uint8 s2, uint8 s3, uint16 a)
{
	blend_composite(d1, d2, d3, da, s1, s2, s3, a);
}

// assign_alpha_pc
inline void
assign_alpha_pc(uint8* d1, uint8* d2, uint8* d3, uint8* da,
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
	class DrawingModeAlphaPC : public DrawingMode
	{
	public:
		typedef Order order_type;

		// constructor
		DrawingModeAlphaPC()
			: DrawingMode()
		{
		}

		// blend_pixel
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			uint16 alpha = color.alpha * cover;
			if (alpha == 255*255) {
				assign_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								color.red, color.green, color.blue);
			} else {
				blend_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
							   color.red, color.green, color.blue, alpha);
			}
		}

		// blend_hline
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
				uint16 alpha = color.alpha * cover;
				if (alpha) {
					if (alpha == 255) {
						assign_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										color.red, color.green, color.blue);
					} else {
						blend_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									   color.red, color.green, color.blue, alpha);
					}
				}
				x++;
				p += 4;
			} while(--len);
		}

		// blend_vline
		virtual	void blend_vline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
printf("DrawingModeAlphaPC::blend_vline()\n");
		}

		// blend_solid_hspan
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
				uint16 alpha = color.alpha * *covers;
				if (alpha) {
					if(alpha == 255*255) {
						assign_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										color.red, color.green, color.blue);
					} else {
						blend_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
				uint16 alpha = color.alpha * *covers;
				if (alpha) {
					if (alpha == 255*255) {
						assign_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									color.red, color.green, color.blue);
					} else {
						blend_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
			if (covers) {
				// non-solid opacity
				do {
					uint16 alpha = colors->a * *covers;
					if (alpha) {
						if (alpha == 255*255) {
							assign_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										colors->r, colors->g, colors->b);
						} else {
							blend_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
						assign_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									colors->r, colors->g, colors->b);
						p += 4;
						++colors;
					} while(--len);
				// solid partial opacity
				} else if (alpha) {
					do {
						blend_alpha_pc(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
printf("DrawingModeAlphaPC::blend_color_vspan()\n");
		}

	};

	typedef DrawingModeAlphaPC<order_rgba32> DrawingModeRGBA32AlphaPC; //----DrawingModeRGBA32AlphaPC
	typedef DrawingModeAlphaPC<order_argb32> DrawingModeARGB32AlphaPC; //----DrawingModeARGB32AlphaPC
	typedef DrawingModeAlphaPC<order_abgr32> DrawingModeABGR32AlphaPC; //----DrawingModeABGR32AlphaPC
	typedef DrawingModeAlphaPC<order_bgra32> DrawingModeBGRA32AlphaPC; //----DrawingModeBGRA32AlphaPC
}

#endif // DRAWING_MODE_ALPHA_PC_H

