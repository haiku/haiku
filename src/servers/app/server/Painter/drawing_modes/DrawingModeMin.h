// DrawingModeMin.h

#ifndef DRAWING_MODE_MIN_H
#define DRAWING_MODE_MIN_H

#include <SupportDefs.h>

#include "DrawingMode.h"
#include "PatternHandler.h"


inline void
blend_min(uint8* d1, uint8* d2, uint8* d3, uint8* da,
		  uint8 s1, uint8 s2, uint8 s3, uint8 a)
{
	s1 = min_c(*d1, s1);
	s2 = min_c(*d2, s2);
	s3 = min_c(*d3, s3);

	blend(d1, d2, d3, da, s1, s2, s3, a);
}

inline void
assign_min(uint8* d1, uint8* d2, uint8* d3, uint8* da,
		   uint8 s1, uint8 s2, uint8 s3)
{
	*d1 = min_c(*d1, s1);
	*d2 = min_c(*d2, s2);
	*d3 = min_c(*d3, s3);
	*da = 255;
}


namespace agg
{
	template<class Order>
	class DrawingModeMin : public DrawingMode
	{
	public:
		typedef Order order_type;

		// constructor
		DrawingModeMin()
			: DrawingMode()
		{
		}

		// blend_pixel
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			if (cover == 255) {
				assign_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
						   color.red, color.green, color.blue);
			} else {
				blend_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
						  color.red, color.green, color.blue, cover);
			}
		}

		// blend_hline
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			if (cover == 255) {
				do {
					rgb_color color = fPatternHandler->R5ColorAt(x, y);

					assign_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
							   color.red, color.green, color.blue);

					p += 4;
					x++;
				} while(--len);
			} else {
				do {
					rgb_color color = fPatternHandler->R5ColorAt(x, y);

					blend_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
							  color.red, color.green, color.blue, cover);

					x++;
					p += 4;
				} while(--len);
			}
		}

		// blend_vline
		virtual	void blend_vline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
printf("DrawingModeMin::blend_vline()\n");
		}

		// blend_solid_hspan
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
				if (*covers) {
					if (*covers == 255) {
						assign_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								   color.red, color.green, color.blue);
					} else {
						blend_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
				if (*covers) {
					if (*covers == 255) {
						assign_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								   color.red, color.green, color.blue);
					} else {
						blend_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								  color.red, color.green, color.blue, *covers);
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
							assign_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									   colors->r, colors->g, colors->b);
						} else {
							blend_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
						assign_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								   colors->r, colors->g, colors->b);
						p += 4;
						++colors;
					} while(--len);
				// solid partial opacity
				} else if (cover) {
					do {
						blend_min(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
									   const int8u* covers,
									   int8u cover)
		{
printf("DrawingModeMin::blend_color_vspan()\n");
		}

	};

	typedef DrawingModeMin<order_rgba32> DrawingModeRGBA32Min; //----DrawingModeRGBA32Min
	typedef DrawingModeMin<order_argb32> DrawingModeARGB32Min; //----DrawingModeARGB32Min
	typedef DrawingModeMin<order_abgr32> DrawingModeABGR32Min; //----DrawingModeABGR32Min
	typedef DrawingModeMin<order_bgra32> DrawingModeBGRA32Min; //----DrawingModeBGRA32Min
}

#endif // DRAWING_MODE_MIN_H

