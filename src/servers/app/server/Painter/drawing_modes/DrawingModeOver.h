// DrawingModeOver.h

#ifndef DRAWING_MODE_OVER_H
#define DRAWING_MODE_OVER_H

#include "DrawingMode.h"

// blend_over
inline void
blend_over(uint8* d1, uint8* d2, uint8* d3, uint8* da,
		   uint8 s1, uint8 s2, uint8 s3, uint8 a)
{
	blend(d1, d2, d3, da, s1, s2, s3, a);
}

// assign_over
inline void
assign_over(uint8* d1, uint8* d2, uint8* d3, uint8* da,
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
	class DrawingModeOver : public DrawingMode
	{
	public:
		typedef Order order_type;

		// constructor
		DrawingModeOver()
			: DrawingMode()
		{
		}

		// blend_pixel
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
			if (fPatternHandler->IsHighColor(x, y)) {
				int8u* p = m_rbuf->row(y) + (x << 2);
				rgb_color color = fPatternHandler->HighColor().GetColor32();
				if (cover == 255) {
					assign_over(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								color.red, color.green, color.blue);
				} else {
					blend_over(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
							   color.red, color.green, color.blue, cover);
				}
			}
		}

		// blend_hline
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
			if(cover == 255) {
				rgb_color color = fPatternHandler->HighColor().GetColor32();
				int32u v;
				int8u* p8 = (int8u*)&v;
				p8[Order::R] = (int8u)color.red;
				p8[Order::G] = (int8u)color.green;
				p8[Order::B] = (int8u)color.blue;
				p8[Order::A] = 255;
				int32u* p32 = (int32u*)(m_rbuf->row(y)) + x;
				do {
					if (fPatternHandler->IsHighColor(x, y))
						*p32 = v;
					p32++;
					x++;
				} while(--len);
			} else {
				int8u* p = m_rbuf->row(y) + (x << 2);
				rgb_color color = fPatternHandler->HighColor().GetColor32();
				do {
					if (fPatternHandler->IsHighColor(x, y)) {
						blend_over(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								   color.red, color.green, color.blue, cover);
					}
					x++;
					p += 4;
				} while(--len);
			}
		}

		// blend_vline
		virtual	void blend_vline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
printf("DrawingModeOver::blend_vline()\n");
		}

		// blend_solid_hspan
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			rgb_color color = fPatternHandler->HighColor().GetColor32();
			do {
				if (fPatternHandler->IsHighColor(x, y)) {
					if (*covers) {
						if(*covers == 255) {
							assign_over(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										color.red, color.green, color.blue);
						} else {
							blend_over(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			rgb_color color = fPatternHandler->HighColor().GetColor32();
			do {
				if (fPatternHandler->IsHighColor(x, y)) {
					if (*covers) {
						if (*covers == 255) {
							assign_over(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										color.red, color.green, color.blue);
						} else {
							blend_over(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									   color.red, color.green, color.blue, *covers);
						}
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
					if (*covers) {
						if (*covers == 255) {
							assign_over(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										colors->r, colors->g, colors->b);
						} else {
							blend_over(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
						assign_over(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									colors->r, colors->g, colors->b);
						p += 4;
						++colors;
					} while(--len);
				// solid partial opacity
				} else if (cover) {
					do {
						blend_over(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								   colors->r, colors->g, colors->b, cover);
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
printf("DrawingModeOver::blend_color_vspan()\n");
		}

	};

	typedef DrawingModeOver<order_rgba32> DrawingModeRGBA32Over; //----DrawingModeRGBA32Over
	typedef DrawingModeOver<order_argb32> DrawingModeARGB32Over; //----DrawingModeARGB32Over
	typedef DrawingModeOver<order_abgr32> DrawingModeABGR32Over; //----DrawingModeABGR32Over
	typedef DrawingModeOver<order_bgra32> DrawingModeBGRA32Over; //----DrawingModeBGRA32Over
}

#endif // DRAWING_MODE_OVER_H

