// DrawingModeOver.h

#ifndef DRAWING_MODE_OVER_H
#define DRAWING_MODE_OVER_H

#include "DrawingMode.h"

namespace agg
{
	//====================================================DrawingModeOver
	template<class Order>
	class DrawingModeOver : public DrawingMode
	{
	public:
		typedef Order order_type;

		//--------------------------------------------------------------------
		DrawingModeOver()
			: DrawingMode()
		{
		}

		//--------------------------------------------------------------------
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
//printf("DrawingModeOver::blend_pixel()\n");
			if (fPatternHandler->IsHighColor(x, y)) {
				int8u* p = m_rbuf->row(y) + (x << 2);
				rgb_color color = fPatternHandler->ColorAt(x, y);
	//			int alpha = int(cover) * int(c.a);
				int alpha = int(cover) * int(color.alpha);
				if(alpha == 255*255)
				{
	//				p[Order::R] = (int8u)c.r;
	//				p[Order::G] = (int8u)c.g;
	//				p[Order::B] = (int8u)c.b;
	//				p[Order::A] = (int8u)c.a;
	
					p[Order::R] = color.red;
					p[Order::G] = color.green;
					p[Order::B] = color.blue;
					p[Order::A] = color.alpha;
				}
				else
				{
					int r = p[Order::R];
					int g = p[Order::G];
					int b = p[Order::B];
					int a = p[Order::A];
	//				p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
	//				p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
	//				p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
	//				p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
					p[Order::R] = (int8u)((((color.red - r) * alpha) + (r << 16)) >> 16);
					p[Order::G] = (int8u)((((color.green - g) * alpha) + (g << 16)) >> 16);
					p[Order::B] = (int8u)((((color.blue - b) * alpha) + (b << 16)) >> 16);
					p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
				}
			}
		}

		//--------------------------------------------------------------------
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
//printf("DrawingModeOver::blend_hline()\n");
			int alpha = int(cover) * int(c.a);
			if(alpha == 255*255)
			{
				int32u v;
				int8u* p8 = (int8u*)&v;
				p8[Order::R] = (int8u)c.r;
				p8[Order::G] = (int8u)c.g;
				p8[Order::B] = (int8u)c.b;
				p8[Order::A] = (int8u)c.a;
				int32u* p32 = (int32u*)(m_rbuf->row(y)) + x;
				do
				{
					if (fPatternHandler->IsHighColor(x, y))
						*p32 = v;
					p32++;
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
						p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
						p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
						p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
						p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
					}
					x++;
					p += 4;
				}
				while(--len);
			}
		}

		//--------------------------------------------------------------------
		virtual	void blend_vline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
printf("DrawingModeOver::blend_vline()\n");
		}

		//--------------------------------------------------------------------
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
//printf("DrawingModeOver::blend_solid_hspan()\n");
			int8u* p = m_rbuf->row(y) + (x << 2);
			do 
			{
				if (fPatternHandler->IsHighColor(x, y)) {
					rgb_color color = fPatternHandler->ColorAt(x, y);
	//				int alpha = int(*covers++) * c.a;
					int alpha = int(*covers) * color.alpha;
	//int alpha = int(*covers++);
					if(alpha)
					{
						if(alpha == 255*255)
						{
	//						p[Order::R] = (int8u)c.r;
	//						p[Order::G] = (int8u)c.g;
	//						p[Order::B] = (int8u)c.b;
	//						p[Order::A] = (int8u)c.a;
							p[Order::R] = color.red;
							p[Order::G] = color.green;
							p[Order::B] = color.blue;
							p[Order::A] = color.alpha;
						}
						else
						{
							int r = p[Order::R];
							int g = p[Order::G];
							int b = p[Order::B];
							int a = p[Order::A];
	//						p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
	//						p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
	//						p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
	//						p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
							p[Order::R] = (int8u)((((color.red - r) * alpha) + (r << 16)) >> 16);
							p[Order::G] = (int8u)((((color.green - g) * alpha) + (g << 16)) >> 16);
							p[Order::B] = (int8u)((((color.blue - b) * alpha) + (b << 16)) >> 16);
							p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
						}
					}
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
//printf("DrawingModeOver::blend_solid_vspan()\n");
			int8u* p = m_rbuf->row(y) + (x << 2);
			do 
			{
				if (fPatternHandler->IsHighColor(x, y)) {
					rgb_color color = fPatternHandler->ColorAt(x, y);
	//				int alpha = int(*covers++) * c.a;
					int alpha = int(*covers) * color.alpha;
	//int alpha = int(*covers++);
	
					if(alpha)
					{
						if(alpha == 255*255)
						{
	//						p[Order::R] = (int8u)c.r;
	//						p[Order::G] = (int8u)c.g;
	//						p[Order::B] = (int8u)c.b;
	//						p[Order::A] = (int8u)c.a;
							p[Order::R] = color.red;
							p[Order::G] = color.green;
							p[Order::B] = color.blue;
							p[Order::A] = color.alpha;
						}
						else
						{
							int r = p[Order::R];
							int g = p[Order::G];
							int b = p[Order::B];
							int a = p[Order::A];
	//						p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
	//						p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
	//						p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
	//						p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
							p[Order::R] = (int8u)((((color.red - r) * alpha) + (r << 16)) >> 16);
							p[Order::G] = (int8u)((((color.green - g) * alpha) + (g << 16)) >> 16);
							p[Order::B] = (int8u)((((color.blue - b) * alpha) + (b << 16)) >> 16);
							p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
						}
					}
				};
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
//printf("DrawingModeOver::blend_color_hspan()\n");
			int8u* p = m_rbuf->row(y) + (x << 2);
			do 
			{
				int alpha = colors->a * (covers ? int(*covers++) : int(cover));

				if(alpha)
				{
					if(alpha == 255*255)
					{
						p[Order::R] = (int8u)colors->r;
						p[Order::G] = (int8u)colors->g;
						p[Order::B] = (int8u)colors->b;
						p[Order::A] = (int8u)colors->a;
					}
					else
					{
						int r = p[Order::R];
						int g = p[Order::G];
						int b = p[Order::B];
						int a = p[Order::A];
						p[Order::R] = (int8u)((((colors->r - r) * alpha) + (r << 16)) >> 16);
						p[Order::G] = (int8u)((((colors->g - g) * alpha) + (g << 16)) >> 16);
						p[Order::B] = (int8u)((((colors->b - b) * alpha) + (b << 16)) >> 16);
						p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
					}
				}
				p += 4;
				++colors;
			}
			while(--len);
		}


		//--------------------------------------------------------------------
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

