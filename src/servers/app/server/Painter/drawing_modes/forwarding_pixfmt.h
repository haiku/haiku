// forwarding_pixfmt.h

#ifndef FORWARDING_PIXFMT_H
#define FORWARDING_PIXFMT_H

#include <stdio.h>
#include <string.h>

#include <agg_basics.h>
#include <agg_color_rgba8.h>
#include <agg_rendering_buffer.h>

#include "DrawingMode.h"
#include "PatternHandler.h"

namespace agg
{

	//====================================================forwarding_pixel_format
	template<class Order>
	class forwarding_pixel_format
	{
	public:
		typedef rgba8 color_type;
		typedef Order order_type;
		typedef rendering_buffer::row_data row_data;

		//--------------------------------------------------------------------
		forwarding_pixel_format(rendering_buffer& rb, const PatternHandler* handler)
			: fBuffer(&rb),
			  fDrawingMode(NULL),
			  fPatternHandler(handler)
		{
		}

		~forwarding_pixel_format()
		{
			delete fDrawingMode;
		}

		//--------------------------------------------------------------------
		void set_drawing_mode(DrawingMode* mode)
		{
//printf("forwarding_pixel_format::set_drawing_mode()\n");
			if (fDrawingMode != mode) {
				// delete old DrawingMode
				delete fDrawingMode;
				// attach new DrawingMode
				fDrawingMode = mode;
				if (fDrawingMode) {
					fDrawingMode->set_rendering_buffer(fBuffer);
					fDrawingMode->set_pattern_handler(fPatternHandler);
				}
			}
		}

		//--------------------------------------------------------------------
		void set_pattern(const PatternHandler* handler)
		{
			fPatternHandler = handler;
		}

		//--------------------------------------------------------------------
		unsigned width()  const { return fBuffer->width();  }
		unsigned height() const { return fBuffer->height(); }

		//--------------------------------------------------------------------
		color_type pixel(int x, int y) const
		{
printf("forwarding_pixel_format::pixel()\n");
			int8u* p = fBuffer->row(y) + (x << 2);
			return color_type(p[Order::R], p[Order::G], p[Order::B], p[Order::A]);
		}

		//--------------------------------------------------------------------
		row_data span(int x, int y) const
		{
printf("forwarding_pixel_format::span()\n");
			return row_data(x, width() - 1, fBuffer->row(y) + (x << 2));
		}

		//--------------------------------------------------------------------
		void copy_pixel(int x, int y, const color_type& c)
		{
printf("forwarding_pixel_format::copy_pixel()\n");
			int8u* p = fBuffer->row(y) + (x << 2);
			p[Order::R] = (int8u)c.r;
			p[Order::G] = (int8u)c.g;
			p[Order::B] = (int8u)c.b;
			p[Order::A] = (int8u)c.a;
		}

		//--------------------------------------------------------------------
		void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
//printf("forwarding_pixel_format::blend_pixel()\n");
			fDrawingMode->blend_pixel(x, y, c, cover);
		}

		//--------------------------------------------------------------------
		void copy_hline(int x, int y, unsigned len, const color_type& c)
		{
printf("forwarding_pixel_format::copy_hline()\n");
			int32u v;
			int8u* p8 = (int8u*)&v;
			p8[Order::R] = (int8u)c.r;
			p8[Order::G] = (int8u)c.g;
			p8[Order::B] = (int8u)c.b;
			p8[Order::A] = (int8u)c.a;
			int32u* p32 = (int32u*)(fBuffer->row(y)) + x;
			do
			{
				*p32++ = v;
			}
			while(--len);
		}

		//--------------------------------------------------------------------
		void copy_vline(int x, int y, unsigned len, const color_type& c)
		{
printf("forwarding_pixel_format::copy_vline()\n");
			int32u v;
			int8u* p8 = (int8u*)&v;
			p8[Order::R] = (int8u)c.r;
			p8[Order::G] = (int8u)c.g;
			p8[Order::B] = (int8u)c.b;
			p8[Order::A] = (int8u)c.a;
			int8u* p = fBuffer->row(y) + (x << 2);
			do
			{
				*(int32u*)p = v; 
				p += fBuffer->stride();
			}
			while(--len);
		}

		//--------------------------------------------------------------------
		void blend_hline(int x, int y, unsigned len, 
						 const color_type& c, int8u cover)
		{
//printf("forwarding_pixel_format::blend_hline()\n");
			fDrawingMode->blend_hline(x, y, len, c, cover);
		}

		//--------------------------------------------------------------------
		void blend_vline(int x, int y, unsigned len, 
						 const color_type& c, int8u cover)
		{
printf("forwarding_pixel_format::blend_vline()\n");
			fDrawingMode->blend_vline(x, y, len, c, cover);
		}


		//--------------------------------------------------------------------
		void copy_from(const rendering_buffer& from, 
					   int xdst, int ydst,
					   int xsrc, int ysrc,
					   unsigned len)
		{
printf("forwarding_pixel_format::copy_from()\n");
			memmove(fBuffer->row(ydst) + xdst * 4, 
					(const void*)(from.row(ysrc) + xsrc * 4), len * 4);
		}



		//--------------------------------------------------------------------
		template<class SrcPixelFormatRenderer>
		void blend_from(const SrcPixelFormatRenderer& from, 
						const int8u* psrc,
						int xdst, int ydst,
						int xsrc, int ysrc,
						unsigned len)
		{
printf("forwarding_pixel_format::blend_from()\n");
			typedef typename SrcPixelFormatRenderer::order_type src_order;

			int8u* pdst = fBuffer->row(ydst) + (xdst << 2);
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
						pdst[Order::R] = (int8u)((((psrc[src_order::R] - r) * alpha) + (r << 8)) >> 8);
						pdst[Order::G] = (int8u)((((psrc[src_order::G] - g) * alpha) + (g << 8)) >> 8);
						pdst[Order::B] = (int8u)((((psrc[src_order::B] - b) * alpha) + (b << 8)) >> 8);
						pdst[Order::A] = (int8u)((alpha + a) - ((alpha * a) >> 8));
					}
				}
				psrc += incp;
				pdst += incp;
			}
			while(--len);
		}




		//--------------------------------------------------------------------
		void blend_solid_hspan(int x, int y, unsigned len, 
							   const color_type& c, const int8u* covers)
		{
//printf("forwarding_pixel_format::blend_solid_hspan()\n");
			fDrawingMode->blend_solid_hspan(x, y, len, c, covers);
		}



		//--------------------------------------------------------------------
		void blend_solid_vspan(int x, int y, unsigned len, 
							   const color_type& c, const int8u* covers)
		{
//printf("forwarding_pixel_format::blend_solid_vspan()\n");
			fDrawingMode->blend_solid_vspan(x, y, len, c, covers);
		}


		//--------------------------------------------------------------------
		void blend_color_hspan(int x, int y, unsigned len, 
							   const color_type* colors, 
							   const int8u* covers,
							   int8u cover)
		{
//printf("forwarding_pixel_format::blend_color_hspan()\n");
			fDrawingMode->blend_color_hspan(x, y, len, colors, covers, cover);
		}


		//--------------------------------------------------------------------
		void blend_color_vspan(int x, int y, unsigned len, 
							   const color_type* colors, 
							   const int8u* covers,
							   int8u cover)
		{
printf("forwarding_pixel_format::blend_color_vspan()\n");
			fDrawingMode->blend_color_vspan(x, y, len, colors, covers, cover);
		}

	private:
		rendering_buffer*		fBuffer;

		DrawingMode*			fDrawingMode;
		const PatternHandler*	fPatternHandler;

	};

}

#endif // FORWARDING_PIXFMT_H

