// DrawingModeCopySolid.h

#ifndef DRAWING_MODE_COPY_SOLID_H
#define DRAWING_MODE_COPY_SOLID_H

#include "DrawingModeCopySolid.h"

namespace agg
{
	template<class Order>
	class DrawingModeCopySolid : public DrawingMode
	{
	public:
		typedef Order order_type;

		// constructor
		DrawingModeCopySolid()
			: DrawingMode()
		{
		}

		// blend_pixel
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			if (cover == 255) {
				assign_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
							c.r, c.g, c.b);
			} else {
				blend_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
						   c.r, c.g, c.b, cover);
			}
		}

		// blend_hline
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
			if(cover == 255) {
				// cache the color as 32bit value
				int32u v;
				int8u* p8 = (int8u*)&v;
				p8[Order::R] = (int8u)c.r;
				p8[Order::G] = (int8u)c.g;
				p8[Order::B] = (int8u)c.b;
				p8[Order::A] = 255;
				// row offset as 32bit pointer
				int32u* p32 = (int32u*)(m_rbuf->row(y)) + x;
				do {
					*p32 = v;
					p32++;
				} while(--len);
			} else {
				int8u* p = m_rbuf->row(y) + (x << 2);
				do {
					blend_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
							   c.r, c.g, c.b, cover);
					p += 4;
				} while(--len);
			}
		}

		// blend_vline
		virtual	void blend_vline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
printf("DrawingModeCopySolid::blend_vline()\n");
		}

		// blend_solid_hspan
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			do {
				if (*covers) {
					if(*covers == 255) {
						assign_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									c.r, c.g, c.b);
					} else {
						blend_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								   c.r, c.g, c.b, *covers);
					}
				}
				covers++;
				p += 4;
			} while(--len);
		}



		// blend_solid_vspan
		virtual	void blend_solid_vspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			do {
				if (*covers) {
					if (*covers == 255) {
						assign_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									c.r, c.g, c.b);
					} else {
						blend_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
								   c.r, c.g, c.b, *covers);
					}
				}
				covers++;
				p += m_rbuf->stride();
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
					if(*covers) {
						if(*covers == 255) {
							assign_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										colors->r, colors->g, colors->b);
						} else {
							blend_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
						assign_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									colors->r, colors->g, colors->b);
						p += 4;
						++colors;
					} while(--len);
				// solid partial opacity
				} else if (cover) {
					do {
						blend_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
printf("DrawingModeCopySolid::blend_color_vspan()\n");
		}

	};

	typedef DrawingModeCopySolid<order_rgba32> DrawingModeRGBA32CopySolid; //----DrawingModeRGBA32CopySolid
	typedef DrawingModeCopySolid<order_argb32> DrawingModeARGB32CopySolid; //----DrawingModeARGB32CopySolid
	typedef DrawingModeCopySolid<order_abgr32> DrawingModeABGR32CopySolid; //----DrawingModeABGR32CopySolid
	typedef DrawingModeCopySolid<order_bgra32> DrawingModeBGRA32CopySolid; //----DrawingModeBGRA32CopySolid
}

#endif // DRAWING_MODE_COPY_SOLID_H

