// DrawingModeCopy.h

#ifndef DRAWING_MODE_COPY_H
#define DRAWING_MODE_COPY_H

#include "DrawingMode.h"

// blend_copy
inline void
blend_copy(uint8* d1, uint8* d2, uint8* d3, uint8* da,
		   uint8 s1, uint8 s2, uint8 s3, uint8 a)
{
	blend(d1, d2, d3, da, s1, s2, s3, a);
}

// assign_copy
inline void
assign_copy(uint8* d1, uint8* d2, uint8* d3, uint8* da,
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
	class DrawingModeCopy : public DrawingMode
	{
	public:
		typedef Order order_type;

		// constructor
		DrawingModeCopy()
			: DrawingMode()
		{
		}

		// blend_pixel
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
			if (cover == 255) {
				assign_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
							color.red, color.green, color.blue);
			} else {
				blend_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
						   color.red, color.green, color.blue, cover);
			}
		}

		// blend_hline
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
			if(cover == 255) {
				// cache the low and high color as 32bit values
				// high color
				rgb_color color = fPatternHandler->HighColor().GetColor32();
				int32u vh;
				int8u* p8 = (int8u*)&vh;
				p8[Order::R] = (int8u)color.red;
				p8[Order::G] = (int8u)color.green;
				p8[Order::B] = (int8u)color.blue;
				p8[Order::A] = 255;
				// low color
				color = fPatternHandler->LowColor().GetColor32();
				int32u vl;
				p8 = (int8u*)&vl;
				p8[Order::R] = (int8u)color.red;
				p8[Order::G] = (int8u)color.green;
				p8[Order::B] = (int8u)color.blue;
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
					blend_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
printf("DrawingModeCopy::blend_vline()\n");
		}

		// blend_solid_hspan
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			do {
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
				if (*covers) {
					if(*covers == 255) {
						assign_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									color.red, color.green, color.blue);
					} else {
						blend_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
						assign_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									color.red, color.green, color.blue);
					} else {
						blend_copy(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
printf("DrawingModeCopy::blend_color_vspan()\n");
		}

	};

	typedef DrawingModeCopy<order_rgba32> DrawingModeRGBA32Copy; //----DrawingModeRGBA32Copy
	typedef DrawingModeCopy<order_argb32> DrawingModeARGB32Copy; //----DrawingModeARGB32Copy
	typedef DrawingModeCopy<order_abgr32> DrawingModeABGR32Copy; //----DrawingModeABGR32Copy
	typedef DrawingModeCopy<order_bgra32> DrawingModeBGRA32Copy; //----DrawingModeBGRA32Copy
}

#endif // DRAWING_MODE_COPY_H

