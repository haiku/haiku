// DrawingModeInvert.h

#ifndef DRAWING_MODE_INVERT_H
#define DRAWING_MODE_INVERT_H

#include "DrawingMode.h"

// BLEND_INVERT
#define BLEND_INVERT(d1, d2, d3, da, a) \
{ \
	BLEND(d1, d2, d3, da, 255 - d1, 255 - d2, 255 - d3, a); \
}

// ASSIGN_INVERT
#define ASSIGN_INVERT(d1, d2, d3, da) \
{ \
	(d1) = 255 - (d1); \
	(d2) = 255 - (d2); \
	(d3) = 255 - (d3); \
	(da) = 255; \
}


template<class Order>
class DrawingModeInvert : public DrawingMode {
 public:
	// constructor
	DrawingModeInvert()
		: DrawingMode()
	{
	}

	// blend_pixel
	virtual	void blend_pixel(int x, int y, const color_type& c, uint8 cover)
	{
		if (fPatternHandler->IsHighColor(x, y)) {
			uint8* p = fBuffer->row(y) + (x << 2);
			if (cover == 255) {
				ASSIGN_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A]);
			} else {
				BLEND_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A], cover);
			}
		}
	}

	// blend_hline
	virtual	void blend_hline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		if(cover == 255) {
			do {
				if (fPatternHandler->IsHighColor(x, y)) {
					ASSIGN_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A]);
				}
				x++;
				p += 4;
			} while(--len);
		} else {
			do {
				if (fPatternHandler->IsHighColor(x, y)) {
					BLEND_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A], cover);
				}
				x++;
				p += 4;
			} while(--len);
		}
	}

	// blend_vline
	virtual	void blend_vline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
printf("DrawingModeInvert::blend_vline()\n");
	}

	// blend_solid_hspan
	virtual	void blend_solid_hspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		do {
			if (fPatternHandler->IsHighColor(x, y)) {
				if (*covers) {
					if (*covers == 255) {
						ASSIGN_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A]);
					} else {
						BLEND_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A], *covers);
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
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		do {
			if (fPatternHandler->IsHighColor(x, y)) {
				if (*covers) {
					if (*covers == 255) {
						ASSIGN_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A]);
					} else {
						BLEND_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A], *covers);
					}
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
		// TODO: does the R5 app_server check for the
		// appearance of the high color in the source bitmap?
		uint8* p = fBuffer->row(y) + (x << 2);
		if (covers) {
			// non-solid opacity
			do {
				if (*covers) {
					if (*covers == 255) {
						ASSIGN_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A]);
					} else {
						BLEND_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A], *covers);
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
					ASSIGN_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A]);
					p += 4;
//						++colors;
				} while(--len);
			// solid partial opacity
			} else if (cover) {
				do {
					BLEND_INVERT(p[Order::R], p[Order::G], p[Order::B], p[Order::A], cover);
					p += 4;
//						++colors;
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
printf("DrawingModeInvert::blend_color_vspan()\n");
	}

};

typedef DrawingModeInvert<agg::order_rgba32> DrawingModeRGBA32Invert;
typedef DrawingModeInvert<agg::order_argb32> DrawingModeARGB32Invert;
typedef DrawingModeInvert<agg::order_abgr32> DrawingModeABGR32Invert;
typedef DrawingModeInvert<agg::order_bgra32> DrawingModeBGRA32Invert;

#endif // DRAWING_MODE_INVERT_H

