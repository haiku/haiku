// DrawingModeCopy.h

#ifndef DRAWING_MODE_COPY_H
#define DRAWING_MODE_COPY_H

#include "DrawingMode.h"
#include "PatternHandler.h"

namespace agg
{
	//====================================================DrawingModeCopy
	template<class Order>
	class DrawingModeCopy : public DrawingMode
	{
	public:
		typedef Order order_type;

		//--------------------------------------------------------------------
		DrawingModeCopy()
			: DrawingMode()
		{
		}

		//--------------------------------------------------------------------
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
//printf("DrawingModeCopy::blend_pixel()\n");
			int8u* p = m_rbuf->row(y) + (x << 2);
			rgb_color color = fPatternHandler->R5ColorAt(x, y);
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

		//--------------------------------------------------------------------
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
//printf("DrawingModeCopy::blend_hline()\n");
			int alpha = int(cover) * int(c.a);
			if(alpha == 255*255)
			{
				int8u* p = m_rbuf->row(y) + (x << 2);
				do
				{
					rgb_color color = fPatternHandler->R5ColorAt(x, y);
					p[Order::R] = color.red;
					p[Order::G] = color.green;
					p[Order::B] = color.blue;
					p[Order::A] = color.alpha;
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
					rgb_color color = fPatternHandler->R5ColorAt(x, y);
					int r = p[Order::R];
					int g = p[Order::G];
					int b = p[Order::B];
					int a = p[Order::A];
//					p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
//					p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
//					p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
//					p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
					p[Order::R] = (int8u)((((color.red - r) * alpha) + (r << 16)) >> 16);
					p[Order::G] = (int8u)((((color.green - g) * alpha) + (g << 16)) >> 16);
					p[Order::B] = (int8u)((((color.blue - b) * alpha) + (b << 16)) >> 16);
					p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
					p += 4;
					x++;
				}
				while(--len);
			}
		}

		//--------------------------------------------------------------------
		virtual	void blend_vline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
printf("DrawingModeCopy::blend_vline()\n");
		}

		//--------------------------------------------------------------------
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
//printf("DrawingModeCopy::blend_solid_hspan()\n");
			int8u* p = m_rbuf->row(y) + (x << 2);
			do 
			{
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
//				int alpha = int(*covers++) * c.a;
				int alpha = int(*covers++) * color.alpha;
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
/*if(alpha > 127)
{
	p[Order::R] = (int8u)c.r;
	p[Order::G] = (int8u)c.g;
	p[Order::B] = (int8u)c.b;
	p[Order::A] = (int8u)c.a;
}*/
				}
				p += 4;
				x++;
			}
			while(--len);
		}



		//--------------------------------------------------------------------
		virtual	void blend_solid_vspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
//printf("DrawingModeCopy::blend_solid_vspan()\n");
			int8u* p = m_rbuf->row(y) + (x << 2);
			do 
			{
				rgb_color color = fPatternHandler->R5ColorAt(x, y);
//				int alpha = int(*covers++) * c.a;
				int alpha = int(*covers++) * color.alpha;
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
/*if(alpha > 127)
{
	p[Order::R] = (int8u)c.r;
	p[Order::G] = (int8u)c.g;
	p[Order::B] = (int8u)c.b;
	p[Order::A] = (int8u)c.a;
}*/
				}
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
printf("DrawingModeCopy::blend_color_hspan()\n");
		}


		//--------------------------------------------------------------------
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

