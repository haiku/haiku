// DrawingModeInvert.h

#ifndef DRAWING_MODE_INVERT_H
#define DRAWING_MODE_INVERT_H

#include "DrawingMode.h"

#define BINARY 1

namespace agg
{
	//====================================================DrawingModeInvert
	template<class Order>
	class DrawingModeInvert : public DrawingMode
	{
	public:
		typedef Order order_type;

		//--------------------------------------------------------------------
		DrawingModeInvert()
			: DrawingMode()
		{
		}

		//--------------------------------------------------------------------
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
//printf("DrawingModeInvert::blend_pixel()\n");
			if (fPatternHandler->IsHighColor(x, y)) {
				int8u* p = m_rbuf->row(y) + (x << 2);
#if BINARY
				int alpha = int(cover);
				if(alpha > 127)
				{
					p[Order::R] = 255 - p[Order::R];
					p[Order::G] = 255 - p[Order::G];
					p[Order::B] = 255 - p[Order::B];
				//	p[Order::A] = 255 - p[Order::A];
				}
#else
				int alpha = int(cover) * int(c.a);
				if(alpha == 255*255)
				{
					p[Order::R] = 255 - p[Order::R];
					p[Order::G] = 255 - p[Order::G];
					p[Order::B] = 255 - p[Order::B];
//					p[Order::A] = 255 - p[Order::A];
				}
				else
				{
					int r = p[Order::R];
					int g = p[Order::G];
					int b = p[Order::B];
					int a = p[Order::A];
					p[Order::R] = (int8u)(((((255 - r) - r) * alpha) + (r << 16)) >> 16);
					p[Order::G] = (int8u)(((((255 - g) - g) * alpha) + (g << 16)) >> 16);
					p[Order::B] = (int8u)(((((255 - b) - b) * alpha) + (b << 16)) >> 16);
					p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
				}
#endif
			}
		}

		//--------------------------------------------------------------------
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
//printf("DrawingModeInvert::blend_hline()\n");
#if BINARY
			if (cover > 127) {
				int8u* p = m_rbuf->row(y) + (x << 2);
				do
				{
					if (fPatternHandler->IsHighColor(x, y)) {
						p[Order::R] = 255 - p[Order::R];
						p[Order::G] = 255 - p[Order::G];
						p[Order::B] = 255 - p[Order::B];
//						p[Order::A] = 255 - p[Order::A];
					}
					p += 4;
					x++;
				}
				while(--len);
			}
#else
			int alpha = int(cover) * int(c.a);
			if(alpha == 255*255)
			{
				int8u* p = m_rbuf->row(y) + (x << 2);
				do
				{
					if (fPatternHandler->IsHighColor(x, y)) {
						p[Order::R] = 255 - p[Order::R];
						p[Order::G] = 255 - p[Order::G];
						p[Order::B] = 255 - p[Order::B];
//						p[Order::A] = 255 - p[Order::A];
					}
					p += 4;
					x++;
				}
				while(--len);
			}
			else
			{
				int8u* p = m_rbuf->row(y) + (x << 2);
				do
				{
					if (fPatternHandler->IsHighColor(x, y)) {
						int r = p[Order::R];
						int g = p[Order::G];
						int b = p[Order::B];
						int a = p[Order::A];
						p[Order::R] = (int8u)(((((255 - r) - r) * alpha) + (r << 16)) >> 16);
						p[Order::G] = (int8u)(((((255 - g) - g) * alpha) + (g << 16)) >> 16);
						p[Order::B] = (int8u)(((((255 - b) - b) * alpha) + (b << 16)) >> 16);
						p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
					}
					x++;
					p += 4;
				}
				while(--len);
			}
#endif
		}

		//--------------------------------------------------------------------
		virtual	void blend_vline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
printf("DrawingModeInvert::blend_vline()\n");
		}

		//--------------------------------------------------------------------
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
//printf("DrawingModeInvert::blend_solid_hspan()\n");
			int8u* p = m_rbuf->row(y) + (x << 2);
			do 
			{
				if (fPatternHandler->IsHighColor(x, y)) {
#if BINARY
					int alpha = int(*covers);
					if(alpha > 127)
					{
						p[Order::R] = 255 - p[Order::R];
						p[Order::G] = 255 - p[Order::G];
						p[Order::B] = 255 - p[Order::B];
					//	p[Order::A] = 255 - p[Order::A];
					}
#else
					int alpha = int(*covers) * c.a;
					if(alpha)
					{
						if(alpha == 255*255)
						{
							p[Order::R] = 255 - p[Order::R];
							p[Order::G] = 255 - p[Order::G];
							p[Order::B] = 255 - p[Order::B];
//							p[Order::A] = 255 - p[Order::A];
						}
						else
						{
							int r = p[Order::R];
							int g = p[Order::G];
							int b = p[Order::B];
							int a = p[Order::A];
							p[Order::R] = (int8u)(((((255 - r) - r) * alpha) + (r << 16)) >> 16);
							p[Order::G] = (int8u)(((((255 - g) - g) * alpha) + (g << 16)) >> 16);
							p[Order::B] = (int8u)(((((255 - b) - b) * alpha) + (b << 16)) >> 16);
							p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
						}
					}
#endif
				}
				covers++;
				p += 4;
				x++;
			}
			while(--len);
		}



		//--------------------------------------------------------------------
		virtual	void blend_solid_vspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
//printf("DrawingModeInvert::blend_solid_vspan()\n");
			int8u* p = m_rbuf->row(y) + (x << 2);
			do 
			{
				if (fPatternHandler->IsHighColor(x, y)) {
#if BINARY
					int alpha = int(*covers);
					if(alpha > 127)
					{
						p[Order::R] = 255 - p[Order::R];
						p[Order::G] = 255 - p[Order::G];
						p[Order::B] = 255 - p[Order::B];
					//	p[Order::A] = 255 - p[Order::A];
					}
#else
					int alpha = int(*covers) * c.a;
					if(alpha)
					{
						if(alpha == 255*255)
						{
							p[Order::R] = 255 - p[Order::R];
							p[Order::G] = 255 - p[Order::G];
							p[Order::B] = 255 - p[Order::B];
//							p[Order::A] = 255 - p[Order::A];
						}
						else
						{
							int r = p[Order::R];
							int g = p[Order::G];
							int b = p[Order::B];
							int a = p[Order::A];
							p[Order::R] = (int8u)(((((255 - r) - r) * alpha) + (r << 16)) >> 16);
							p[Order::G] = (int8u)(((((255 - g) - g) * alpha) + (g << 16)) >> 16);
							p[Order::B] = (int8u)(((((255 - b) - b) * alpha) + (b << 16)) >> 16);
							p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
						}
					}
#endif
				}
				covers++;
				p += m_rbuf->stride();
				y++;
			}
			while(--len);
		}


		//--------------------------------------------------------------------
		virtual	void blend_color_hspan(int x, int y, unsigned len, 
									   const color_type* colors, 
									   const int8u* covers,
									   int8u cover)
		{
printf("DrawingModeInvert::blend_color_hspan()\n");
		}


		//--------------------------------------------------------------------
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

