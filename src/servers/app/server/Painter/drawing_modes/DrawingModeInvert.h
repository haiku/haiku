// DrawingModeInvert.h

#ifndef DRAWING_MODE_INVERT_H
#define DRAWING_MODE_INVERT_H

#include "DrawingMode.h"

// blend_invert
inline void
blend_invert(uint8* d1, uint8* d2, uint8* d3, uint8* da, uint8 a)
{
	blend(d1, d2, d3, da, 255 - *d1, 255 - *d2, 255 - *d3, a);
}

// assign_invert
inline void
assign_invert(uint8* d1, uint8* d2, uint8* d3, uint8* da)
{
	*d1 = 255 - *d1;
	*d2 = 255 - *d2;
	*d3 = 255 - *d3;
	*da = 255;
}


namespace agg
{
	template<class Order>
	class DrawingModeInvert : public DrawingMode
	{
	public:
		typedef Order order_type;

		// constructor
		DrawingModeInvert()
			: DrawingMode()
		{
		}

		// blend_pixel
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
			if (fPatternHandler->IsHighColor(x, y)) {
				int8u* p = m_rbuf->row(y) + (x << 2);
				if (cover == 255) {
					assign_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A]);
				} else {
					blend_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A], cover);
				}
			}
		}

		// blend_hline
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			if(cover == 255) {
				do {
					if (fPatternHandler->IsHighColor(x, y)) {
						assign_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A]);
					}
					x++;
					p += 4;
				} while(--len);
			} else {
				do {
					if (fPatternHandler->IsHighColor(x, y)) {
						blend_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A], cover);
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
printf("DrawingModeInvert::blend_vline()\n");
		}

		// blend_solid_hspan
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			do {
				if (fPatternHandler->IsHighColor(x, y)) {
					if (*covers) {
						if(*covers == 255) {
							assign_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A]);
						} else {
							blend_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A], *covers);
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
			do {
				if (fPatternHandler->IsHighColor(x, y)) {
					if (*covers) {
						if (*covers == 255) {
							assign_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A]);
						} else {
							blend_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A], *covers);
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
			// TODO: does the R5 app_server check for the
			// appearance of the high color in the source bitmap?
			int8u* p = m_rbuf->row(y) + (x << 2);
			if (covers) {
				// non-solid opacity
				do {
					if(*covers) {
						if(*covers == 255) {
							assign_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A]);
						} else {
							blend_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A], *covers);
						}
					}
					covers++;
					p += 4;
//					++colors;
				} while(--len);
			} else {
				// solid full opcacity
				if (cover == 255) {
					do {
						assign_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A]);
						p += 4;
//						++colors;
					} while(--len);
				// solid partial opacity
				} else if (cover) {
					do {
						blend_invert(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A], cover);
						p += 4;
//						++colors;
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
printf("DrawingModeInvert::blend_color_vspan()\n");
		}

	};

	typedef DrawingModeInvert<order_rgba32> DrawingModeRGBA32Invert; //----DrawingModeRGBA32Invert
	typedef DrawingModeInvert<order_argb32> DrawingModeARGB32Invert; //----DrawingModeARGB32Invert
	typedef DrawingModeInvert<order_abgr32> DrawingModeABGR32Invert; //----DrawingModeABGR32Invert
	typedef DrawingModeInvert<order_bgra32> DrawingModeBGRA32Invert; //----DrawingModeBGRA32Invert
}

#endif // DRAWING_MODE_INVERT_H

