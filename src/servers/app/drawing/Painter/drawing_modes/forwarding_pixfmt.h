/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
 *
 * A class implementing the AGG "pixel format" interface which maintains
 * a PatternHandler and DrawingMode instance, to which it forwards any requests.
 *
 */

#ifndef FORWARDING_PIXFMT_H
#define FORWARDING_PIXFMT_H

#include <stdio.h>
#include <string.h>

#include <agg_basics.h>
#include <agg_color_rgba8.h>
#include <agg_rendering_buffer.h>

#include "DrawingMode.h"
#include "PatternHandler.h"

template<class Order>
class forwarding_pixel_format {
 public:
	typedef agg::rgba8 color_type;
	typedef Order order_type;
	typedef agg::rendering_buffer::row_data row_data;

	// constructor
	forwarding_pixel_format(agg::rendering_buffer& rb, const PatternHandler* handler)
		: fBuffer(&rb),
		  fDrawingMode(NULL),
		  fPatternHandler(handler)
	{
	}

	~forwarding_pixel_format()
	{
	}

	// set_drawing_mode
	void set_drawing_mode(DrawingMode* mode)
	{
		if (fDrawingMode != mode) {
			// attach new DrawingMode
			fDrawingMode = mode;
			if (fDrawingMode) {
				fDrawingMode->set_rendering_buffer(fBuffer);
				fDrawingMode->set_pattern_handler(fPatternHandler);
			}
		}
	}

	// set_pattern
	void set_pattern(const PatternHandler* handler)
	{
		fPatternHandler = handler;
	}

	// width
	unsigned width()  const { return fBuffer->width();  }
	// height
	unsigned height() const { return fBuffer->height(); }

	// pixel
	color_type pixel(int x, int y) const
	{
printf("forwarding_pixel_format::pixel()\n");
		uint8* p = fBuffer->row(y) + (x << 2);
		return color_type(p[Order::R], p[Order::G], p[Order::B], p[Order::A]);
	}

	// span
	row_data span(int x, int y) const
	{
printf("forwarding_pixel_format::span()\n");
		return row_data(x, width() - 1, fBuffer->row(y) + (x << 2));
	}

	// copy_pixel
	void copy_pixel(int x, int y, const color_type& c)
	{
printf("forwarding_pixel_format::copy_pixel()\n");
		uint8* p = fBuffer->row(y) + (x << 2);
		p[Order::R] = (uint8)c.r;
		p[Order::G] = (uint8)c.g;
		p[Order::B] = (uint8)c.b;
		p[Order::A] = (uint8)c.a;
	}

	// blend_pixel
	void blend_pixel(int x, int y, const color_type& c, uint8 cover)
	{
		fDrawingMode->blend_pixel(x, y, c, cover);
	}

	// copy_hline
	void copy_hline(int x, int y, unsigned len, const color_type& c)
	{
printf("forwarding_pixel_format::copy_hline()\n");
		uint32 v;
		uint8* p8 = (uint8*)&v;
		p8[Order::R] = (uint8)c.r;
		p8[Order::G] = (uint8)c.g;
		p8[Order::B] = (uint8)c.b;
		p8[Order::A] = (uint8)c.a;
		uint32* p32 = (uint32*)(fBuffer->row(y)) + x;
		do {
			*p32++ = v;
		}
		while(--len);
	}

	// copy_vline
	void copy_vline(int x, int y, unsigned len, const color_type& c)
	{
printf("forwarding_pixel_format::copy_vline()\n");
		uint32 v;
		uint8* p8 = (uint8*)&v;
		p8[Order::R] = (uint8)c.r;
		p8[Order::G] = (uint8)c.g;
		p8[Order::B] = (uint8)c.b;
		p8[Order::A] = (uint8)c.a;
		uint8* p = fBuffer->row(y) + (x << 2);
		do {
			*(uint32*)p = v; 
			p += fBuffer->stride();
		}
		while(--len);
	}

	// blend_hline
	void blend_hline(int x, int y, unsigned len, 
					 const color_type& c, uint8 cover)
	{
		fDrawingMode->blend_hline(x, y, len, c, cover);
	}

	// blend_vline
	void blend_vline(int x, int y, unsigned len, 
					 const color_type& c, uint8 cover)
	{
printf("forwarding_pixel_format::blend_vline()\n");
		fDrawingMode->blend_vline(x, y, len, c, cover);
	}


	// copy_from
	void copy_from(const agg::rendering_buffer& from, 
				   int xdst, int ydst,
				   int xsrc, int ysrc,
				   unsigned len)
	{
printf("forwarding_pixel_format::copy_from()\n");
		memmove(fBuffer->row(ydst) + xdst * 4, 
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
printf("forwarding_pixel_format::blend_from()\n");
		typedef typename SrcPixelFormatRenderer::order_type src_order;

		uint8* pdst = fBuffer->row(ydst) + (xdst << 2);
		int incp = 4;
		if (xdst > xsrc) {
			psrc += (len-1) << 2;
			pdst += (len-1) << 2;
			incp = -4;
		}
		do {
			int alpha = psrc[src_order::A];

			if (alpha) {
				if (alpha == 255) {
					pdst[Order::R] = psrc[src_order::R];
					pdst[Order::G] = psrc[src_order::G];
					pdst[Order::B] = psrc[src_order::B];
					pdst[Order::A] = psrc[src_order::A];
				} else {
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
		} while(--len);
	}




	// blend_solid_hspan
	void blend_solid_hspan(int x, int y, unsigned len, 
						   const color_type& c, const uint8* covers)
	{
		fDrawingMode->blend_solid_hspan(x, y, len, c, covers);
	}



	// blend_solid_vspan
	void blend_solid_vspan(int x, int y, unsigned len, 
						   const color_type& c, const uint8* covers)
	{
		fDrawingMode->blend_solid_vspan(x, y, len, c, covers);
	}


	// blend_color_hspan
	void blend_color_hspan(int x, int y, unsigned len, 
						   const color_type* colors, 
						   const uint8* covers,
						   uint8 cover)
	{
		fDrawingMode->blend_color_hspan(x, y, len, colors, covers, cover);
	}


	// blend_color_vspan
	void blend_color_vspan(int x, int y, unsigned len, 
						   const color_type* colors, 
						   const uint8* covers,
						   uint8 cover)
	{
printf("forwarding_pixel_format::blend_color_vspan()\n");
		fDrawingMode->blend_color_vspan(x, y, len, colors, covers, cover);
	}

private:
	agg::rendering_buffer*	fBuffer;

	DrawingMode*			fDrawingMode;
	const PatternHandler*	fPatternHandler;

};

#endif // FORWARDING_PIXFMT_H

