// DrawingModeSelect.h

#ifndef DRAWING_MODE_SELECT_H
#define DRAWING_MODE_SELECT_H

#include "DrawingMode.h"

// blend_select
inline void
blend_select(uint8* d1, uint8* d2, uint8* d3, uint8* da,
			 uint8 s1, uint8 s2, uint8 s3, uint8 a)
{
	blend(d1, d2, d3, da, s1, s2, s3, a);
}

// assign_select
inline void
assign_select(uint8* d1, uint8* d2, uint8* d3, uint8* da,
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
	class DrawingModeSelect : public DrawingMode
	{
	public:
		typedef Order order_type;

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
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
			if (fPatternHandler->IsHighColor(x, y)) {
				int8u* p = m_rbuf->row(y) + (x << 2);
				rgb_color high = fPatternHandler->HighColor().GetColor32();
				rgb_color low = fPatternHandler->LowColor().GetColor32();
				rgb_color color; 
				if (compare(p, high, low, &color)) {
					if (cover == 255) {
						assign_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									  color.red, color.green, color.blue);
					} else {
						blend_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									 color.red, color.green, color.blue, cover);
					}
				}
			}
		}

		// blend_hline
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			rgb_color high = fPatternHandler->HighColor().GetColor32();
			rgb_color low = fPatternHandler->LowColor().GetColor32();
			rgb_color color; 
			if(cover == 255) {
				do {
					if (fPatternHandler->IsHighColor(x, y)
						&& compare(p, high, low, &color))
						assign_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
									  color.red, color.green, color.blue);
					x++;
					p += 4;
				} while(--len);
			} else {
				do {
					if (fPatternHandler->IsHighColor(x, y)
						&& compare(p, high, low, &color)) {
						blend_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
printf("DrawingModeSelect::blend_vline()\n");
		}

		// blend_solid_hspan
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
			int8u* p = m_rbuf->row(y) + (x << 2);
			rgb_color high = fPatternHandler->HighColor().GetColor32();
			rgb_color low = fPatternHandler->LowColor().GetColor32();
			rgb_color color; 
			do {
				if (fPatternHandler->IsHighColor(x, y)) {
					if (*covers && compare(p, high, low, &color)) {
						if (*covers == 255) {
							assign_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										  color.red, color.green, color.blue);
						} else {
							blend_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
			rgb_color high = fPatternHandler->HighColor().GetColor32();
			rgb_color low = fPatternHandler->LowColor().GetColor32();
			rgb_color color; 
			do {
				if (fPatternHandler->IsHighColor(x, y)) {
					if (*covers && compare(p, high, low, &color)) {
						if (*covers == 255) {
							assign_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										  color.red, color.green, color.blue);
						} else {
							blend_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
			rgb_color high = fPatternHandler->HighColor().GetColor32();
			rgb_color low = fPatternHandler->LowColor().GetColor32();
			rgb_color color; 
			if (covers) {
				// non-solid opacity
				do {
					if (*covers && compare(p, high, low, &color)) {
						if (*covers == 255) {
							assign_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										  color.red, color.green, color.blue);
						} else {
							blend_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
							assign_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
										  color.red, color.green, color.blue);
						}
						p += 4;
						++colors;
					} while(--len);
				// solid partial opacity
				} else if (cover) {
					do {
						if (compare(p, high, low, &color)) {
							blend_select(&p[Order::R], &p[Order::G], &p[Order::B], &p[Order::A],
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
									   const int8u* covers,
									   int8u cover)
		{
printf("DrawingModeSelect::blend_color_vspan()\n");
		}

	};

	typedef DrawingModeSelect<order_rgba32> DrawingModeRGBA32Select; //----DrawingModeRGBA32Select
	typedef DrawingModeSelect<order_argb32> DrawingModeARGB32Select; //----DrawingModeARGB32Select
	typedef DrawingModeSelect<order_abgr32> DrawingModeABGR32Select; //----DrawingModeABGR32Select
	typedef DrawingModeSelect<order_bgra32> DrawingModeBGRA32Select; //----DrawingModeBGRA32Select
}

#endif // DRAWING_MODE_SELECT_H

