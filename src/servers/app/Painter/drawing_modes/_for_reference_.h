//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.2
// Copyright (C) 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//			mcseemagg@yahoo.com
//			http://www.antigrain.com
//----------------------------------------------------------------------------

#ifndef PIXFMT_RGBA32_OVER_H
#define PIXFMT_RGBA32_OVER_H

#include <string.h>
#include <agg_basics.h>
#include <agg_color_rgba8.h>
#include <agg_rendering_buffer.h>

class PatternHandler;

class DrawingMode;

template<class Order> class pixel_formats_rgba32 {
 public:
	typedef agg::rgba8 color_type;
	typedef Order order_type;
	typedef agg::rendering_buffer::row_data row_data;

	// constructor
	pixel_formats_rgba32(agg::rendering_buffer& rb, const PatternHandler* handler)
		: m_rbuf(&rb)
	{
	}

	// set_drawing_mode
	void set_drawing_mode(DrawingMode* mode)
	{
		// DUMMY
	}

	// set_pattern
	void set_pattern(const PatternHandler* handler)
	{
		// DUMMY
	}

	// width
	unsigned width()  const { return m_rbuf->width();  }
	// height
	unsigned height() const { return m_rbuf->height(); }

	// pixel
	color_type pixel(int x, int y) const
	{
printf("pixel()\n");
		uint8* p = m_rbuf->row(y) + (x << 2);
		return color_type(p[Order::R], p[Order::G], p[Order::B], p[Order::A]);
	}

	// span
	row_data span(int x, int y) const
	{
printf("span()\n");
		return row_data(x, width() - 1, m_rbuf->row(y) + (x << 2));
	}

	// copy_pixel
	void copy_pixel(int x, int y, const color_type& c)
	{
printf("copy_pixel()\n");
		uint8* p = m_rbuf->row(y) + (x << 2);
		p[Order::R] = (uint8)c.r;
		p[Order::G] = (uint8)c.g;
		p[Order::B] = (uint8)c.b;
		p[Order::A] = (uint8)c.a;
	}

	// blend_pixel
	void blend_pixel(int x, int y, const color_type& c, uint8 cover)
	{
//printf("blend_pixel()\n");
		uint8* p = m_rbuf->row(y) + (x << 2);
p[Order::R] = (uint8)c.r;
p[Order::G] = (uint8)c.g;
p[Order::B] = (uint8)c.b;
/*			int alpha = int(cover) * int(c.a);
		if(alpha == 255*255)
		{
			p[Order::R] = (uint8)c.r;
			p[Order::G] = (uint8)c.g;
			p[Order::B] = (uint8)c.b;
			p[Order::A] = (uint8)c.a;
		}
		else
		{
			int r = p[Order::R];
			int g = p[Order::G];
			int b = p[Order::B];
			int a = p[Order::A];
			p[Order::R] = (uint8)((((c.r - r) * alpha) + (r << 16)) >> 16);
			p[Order::G] = (uint8)((((c.g - g) * alpha) + (g << 16)) >> 16);
			p[Order::B] = (uint8)((((c.b - b) * alpha) + (b << 16)) >> 16);
			p[Order::A] = (uint8)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
		}*/
	}

	// copy_hline
	void copy_hline(int x, int y, unsigned len, const color_type& c)
	{
printf("copy_hline()\n");
		uint32 v;
		uint8* p8 = (uint8*)&v;
		p8[Order::R] = (uint8)c.r;
		p8[Order::G] = (uint8)c.g;
		p8[Order::B] = (uint8)c.b;
		p8[Order::A] = (uint8)c.a;
		uint32* p32 = (uint32*)(m_rbuf->row(y)) + x;
		do
		{
			*p32++ = v;
		}
		while(--len);
	}

	// copy_vline
	void copy_vline(int x, int y, unsigned len, const color_type& c)
	{
printf("copy_vline()\n");
		uint32 v;
		uint8* p8 = (uint8*)&v;
		p8[Order::R] = (uint8)c.r;
		p8[Order::G] = (uint8)c.g;
		p8[Order::B] = (uint8)c.b;
		p8[Order::A] = (uint8)c.a;
		uint8* p = m_rbuf->row(y) + (x << 2);
		do
		{
			*(uint32*)p = v; 
			p += m_rbuf->stride();
		}
		while(--len);
	}

	// blend_hline
	void blend_hline(int x, int y, unsigned len, 
					 const color_type& c, uint8 cover)
	{
printf("blend_hline()\n");
		int alpha = int(cover) * int(c.a);
		if(alpha == 255*255)
		{
			uint32 v;
			uint8* p8 = (uint8*)&v;
			p8[Order::R] = (uint8)c.r;
			p8[Order::G] = (uint8)c.g;
			p8[Order::B] = (uint8)c.b;
			p8[Order::A] = (uint8)c.a;
			uint32* p32 = (uint32*)(m_rbuf->row(y)) + x;
			do
			{
				*p32++ = v;
			}
			while(--len);
		}
		else
		{
			uint8* p = m_rbuf->row(y) + (x << 2);
			do
			{
				int r = p[Order::R];
				int g = p[Order::G];
				int b = p[Order::B];
				int a = p[Order::A];
				p[Order::R] = (uint8)((((c.r - r) * alpha) + (r << 16)) >> 16);
				p[Order::G] = (uint8)((((c.g - g) * alpha) + (g << 16)) >> 16);
				p[Order::B] = (uint8)((((c.b - b) * alpha) + (b << 16)) >> 16);
				p[Order::A] = (uint8)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
				p += 4;
			}
			while(--len);
		}
	}

	// blend_vline
	void blend_vline(int x, int y, unsigned len, 
					 const color_type& c, uint8 cover)
	{
printf("blend_vline()\n");
		uint8* p = m_rbuf->row(y) + (x << 2);
		int alpha = int(cover) * c.a;
		if(alpha == 255*255)
		{
			uint32 v;
			uint8* p8 = (uint8*)&v;
			p8[Order::R] = (uint8)c.r;
			p8[Order::G] = (uint8)c.g;
			p8[Order::B] = (uint8)c.b;
			p8[Order::A] = (uint8)c.a;
			do
			{
				*(uint32*)p = v; 
				p += m_rbuf->stride();
			}
			while(--len);
		}
		else
		{
			do
			{
				int r = p[Order::R];
				int g = p[Order::G];
				int b = p[Order::B];
				int a = p[Order::A];
				p[Order::R] = (uint8)((((c.r - r) * alpha) + (r << 16)) >> 16);
				p[Order::G] = (uint8)((((c.g - g) * alpha) + (g << 16)) >> 16);
				p[Order::B] = (uint8)((((c.b - b) * alpha) + (b << 16)) >> 16);
				p[Order::A] = (uint8)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
				p += m_rbuf->stride();
			}
			while(--len);
		}
	}


	// copy_from
	void copy_from(const agg::rendering_buffer& from, 
				   int xdst, int ydst,
				   int xsrc, int ysrc,
				   unsigned len)
	{
printf("copy_from()\n");
		memmove(m_rbuf->row(ydst) + xdst * 4, 
				(const void*)(from.row(ysrc) + xsrc * 4), len * 4);
	}



	// blend_from
	template<class SrcPixelFormatRenderer>
	void blend_from(const SrcPixelFormatRenderer& from, 
					const uint8* psrc,
					int xdst, int ydst,
					int xsrc, int ysrc,
					unsigned len)
	{
printf("blend_from()\n");
		typedef typename SrcPixelFormatRenderer::order_type src_order;

		uint8* pdst = m_rbuf->row(ydst) + (xdst << 2);
		int incp = 4;
		if(xdst > xsrc)
		{
			psrc += (len-1) << 2;
			pdst += (len-1) << 2;
			incp = -4;
		}
		do 
		{
			int alpha = psrc[src_order::A];

			if(alpha)
			{
				if(alpha == 255)
				{
					pdst[Order::R] = psrc[src_order::R];
					pdst[Order::G] = psrc[src_order::G];
					pdst[Order::B] = psrc[src_order::B];
					pdst[Order::A] = psrc[src_order::A];
				}
				else
				{
					int r = pdst[Order::R];
					int g = pdst[Order::G];
					int b = pdst[Order::B];
					int a = pdst[Order::A];
					pdst[Order::R] = (uint8)((((psrc[src_order::R] - r) * alpha) + (r << 8)) >> 8);
					pdst[Order::G] = (uint8)((((psrc[src_order::G] - g) * alpha) + (g << 8)) >> 8);
					pdst[Order::B] = (uint8)((((psrc[src_order::B] - b) * alpha) + (b << 8)) >> 8);
					pdst[Order::A] = (uint8)((alpha + a) - ((alpha * a) >> 8));
				}
			}
			psrc += incp;
			pdst += incp;
		}
		while(--len);
	}




	// blend_solid_hspan
	void blend_solid_hspan(int x, int y, unsigned len, 
						   const color_type& c, const uint8* covers)
	{
//printf("blend_solid_hspan()\n");
		uint8* p = m_rbuf->row(y) + (x << 2);
		do 
		{
			int alpha = int(*covers++) * c.a;
//int alpha = int(*covers++);
			if(alpha)
			{
				if(alpha == 255*255)
				{
					p[Order::R] = (uint8)c.r;
					p[Order::G] = (uint8)c.g;
					p[Order::B] = (uint8)c.b;
					p[Order::A] = (uint8)c.a;
				}
				else
				{
					int r = p[Order::R];
					int g = p[Order::G];
					int b = p[Order::B];
					int a = p[Order::A];
					p[Order::R] = (uint8)((((c.r - r) * alpha) + (r << 16)) >> 16);
					p[Order::G] = (uint8)((((c.g - g) * alpha) + (g << 16)) >> 16);
					p[Order::B] = (uint8)((((c.b - b) * alpha) + (b << 16)) >> 16);
					p[Order::A] = (uint8)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
				}
/*if(alpha > 127)
{
p[Order::R] = (uint8)c.r;
p[Order::G] = (uint8)c.g;
p[Order::B] = (uint8)c.b;
p[Order::A] = (uint8)c.a;
}*/
			}
			p += 4;
		}
		while(--len);
	}



	// blend_solid_vspan
	void blend_solid_vspan(int x, int y, unsigned len, 
						   const color_type& c, const uint8* covers)
	{
//printf("blend_solid_vspan()\n");
		uint8* p = m_rbuf->row(y) + (x << 2);
		do 
		{
			int alpha = int(*covers++) * c.a;
//int alpha = int(*covers++);

			if(alpha)
			{
				if(alpha == 255*255)
				{
					p[Order::R] = (uint8)c.r;
					p[Order::G] = (uint8)c.g;
					p[Order::B] = (uint8)c.b;
					p[Order::A] = (uint8)c.a;
				}
				else
				{
					int r = p[Order::R];
					int g = p[Order::G];
					int b = p[Order::B];
					int a = p[Order::A];
					p[Order::R] = (uint8)((((c.r - r) * alpha) + (r << 16)) >> 16);
					p[Order::G] = (uint8)((((c.g - g) * alpha) + (g << 16)) >> 16);
					p[Order::B] = (uint8)((((c.b - b) * alpha) + (b << 16)) >> 16);
					p[Order::A] = (uint8)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
				}
/*if(alpha > 127)
{
p[Order::R] = (uint8)c.r;
p[Order::G] = (uint8)c.g;
p[Order::B] = (uint8)c.b;
p[Order::A] = (uint8)c.a;
}*/
			}
			p += m_rbuf->stride();
		}
		while(--len);
	}


	// blend_color_hspan
	void blend_color_hspan(int x, int y, unsigned len, 
						   const color_type* colors, 
						   const uint8* covers,
						   uint8 cover)
	{
printf("blend_color_hspan()\n");
		uint8* p = m_rbuf->row(y) + (x << 2);
		do 
		{
			int alpha = colors->a * (covers ? int(*covers++) : int(cover));

			if(alpha)
			{
				if(alpha == 255*255)
				{
					p[Order::R] = (uint8)colors->r;
					p[Order::G] = (uint8)colors->g;
					p[Order::B] = (uint8)colors->b;
					p[Order::A] = (uint8)colors->a;
				}
				else
				{
					int r = p[Order::R];
					int g = p[Order::G];
					int b = p[Order::B];
					int a = p[Order::A];
					p[Order::R] = (uint8)((((colors->r - r) * alpha) + (r << 16)) >> 16);
					p[Order::G] = (uint8)((((colors->g - g) * alpha) + (g << 16)) >> 16);
					p[Order::B] = (uint8)((((colors->b - b) * alpha) + (b << 16)) >> 16);
					p[Order::A] = (uint8)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
				}
			}
			p += 4;
			++colors;
		}
		while(--len);
	}


	// blend_color_vspan
	void blend_color_vspan(int x, int y, unsigned len, 
						   const color_type* colors, 
						   const uint8* covers,
						   uint8 cover)
	{
printf("blend_color_vspan()\n");
		uint8* p = m_rbuf->row(y) + (x << 2);
		do 
		{
			int alpha = colors->a * (covers ? int(*covers++) : int(cover));

			if(alpha)
			{
				if(alpha == 255*255)
				{
					p[Order::R] = (uint8)colors->r;
					p[Order::G] = (uint8)colors->g;
					p[Order::B] = (uint8)colors->b;
					p[Order::A] = (uint8)colors->a;
				}
				else
				{
					int r = p[Order::R];
					int g = p[Order::G];
					int b = p[Order::B];
					int a = p[Order::A];
					p[Order::R] = (uint8)((((colors->r - r) * alpha) + (r << 16)) >> 16);
					p[Order::G] = (uint8)((((colors->g - g) * alpha) + (g << 16)) >> 16);
					p[Order::B] = (uint8)((((colors->b - b) * alpha) + (b << 16)) >> 16);
					p[Order::A] = (uint8)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
				}
			}
			p += m_rbuf->stride();
			++colors;
		}
		while(--len);
	}

private:
	agg::rendering_buffer* m_rbuf;
};

typedef pixel_formats_rgba32<order_rgba32> pixfmt_rgba32;
typedef pixel_formats_rgba32<order_argb32> pixfmt_argb32;
typedef pixel_formats_rgba32<order_abgr32> pixfmt_abgr32;
typedef pixel_formats_rgba32<order_bgra32> pixfmt_bgra32;

#endif // PIXFMT_RGBA32_OVER_H

