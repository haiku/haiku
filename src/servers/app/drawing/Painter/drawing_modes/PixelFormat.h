/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
 *
 * A class implementing the AGG "pixel format" interface which maintains
 * a PatternHandler and pointers to blending functions implementing the
 * different BeOS "drawing_modes".
 *
 */

#ifndef PIXEL_FORMAT_H
#define PIXEL_FORMAT_H

#include <agg_basics.h>
#include <agg_color_rgba.h>
#include <agg_rendering_buffer.h>
#include <agg_pixfmt_rgba.h>

#include <GraphicsDefs.h>

#include "AggCompOpAdapter.h"

class PatternHandler;

class PixelFormat {
 public:
	typedef agg::rgba8						color_type;
	typedef agg::rendering_buffer			agg_buffer;
	typedef agg::rendering_buffer::row_data	row_data;
	typedef agg::order_bgra					order_type;

	typedef agg::comp_op_rgba_src_in<color_type, order_type>	comp_src_in;
	typedef agg::comp_op_rgba_src_out<color_type, order_type>	comp_src_out;
	typedef agg::comp_op_rgba_src_atop<color_type, order_type>	comp_src_atop;
	typedef agg::comp_op_rgba_dst_over<color_type, order_type>	comp_dst_over;
	typedef agg::comp_op_rgba_dst_in<color_type, order_type>	comp_dst_in;
	typedef agg::comp_op_rgba_dst_out<color_type, order_type>	comp_dst_out;
	typedef agg::comp_op_rgba_dst_atop<color_type, order_type>	comp_dst_atop;
	typedef agg::comp_op_rgba_xor<color_type, order_type>		comp_xor;
	typedef agg::comp_op_rgba_clear<color_type, order_type>		comp_clear;
	typedef agg::comp_op_rgba_difference<color_type, order_type>
		comp_difference;
	typedef agg::comp_op_rgba_lighten<color_type, order_type>	comp_lighten;
	typedef agg::comp_op_rgba_darken<color_type, order_type>	comp_darken;

	typedef AggCompOpAdapter<comp_src_in, agg_buffer>		alpha_src_in;
	typedef AggCompOpAdapter<comp_src_out, agg_buffer>		alpha_src_out;
	typedef AggCompOpAdapter<comp_src_atop, agg_buffer>		alpha_src_atop;
	typedef AggCompOpAdapter<comp_dst_over, agg_buffer>		alpha_dst_over;
	typedef AggCompOpAdapter<comp_dst_in, agg_buffer>		alpha_dst_in;
	typedef AggCompOpAdapter<comp_dst_out, agg_buffer>		alpha_dst_out;
	typedef AggCompOpAdapter<comp_dst_atop, agg_buffer>		alpha_dst_atop;
	typedef AggCompOpAdapter<comp_xor, agg_buffer>			alpha_xor;
	typedef AggCompOpAdapter<comp_clear, agg_buffer>		alpha_clear;
	typedef AggCompOpAdapter<comp_difference, agg_buffer>	alpha_difference;
	typedef AggCompOpAdapter<comp_lighten, agg_buffer>		alpha_lighten;
	typedef AggCompOpAdapter<comp_darken, agg_buffer>		alpha_darken;

	enum base_scale_e
	{
		base_shift = color_type::base_shift,
		base_scale = color_type::base_scale,
		base_mask  = color_type::base_mask,
		pix_width  = 4,
	};

	typedef void (*blend_pixel_f)(int x, int y, const color_type& c,
								  uint8 cover,
								  agg_buffer* buffer,
								  const PatternHandler* pattern);

	typedef void (*blend_line)(int x, int y, unsigned len,
							   const color_type& c, uint8 cover,
							   agg_buffer* buffer,
							   const PatternHandler* pattern);

	typedef void (*blend_solid_span)(int x, int y, unsigned len,
									 const color_type& c,
									 const uint8* covers,
									 agg_buffer* buffer,
									 const PatternHandler* pattern);

	typedef void (*blend_color_span)(int x, int y, unsigned len,
									 const color_type* colors,
									 const uint8* covers,
									 uint8 cover,
									 agg_buffer* buffer,
									 const PatternHandler* pattern);

			// PixelFormat class
								PixelFormat(agg::rendering_buffer& buffer,
											const PatternHandler* handler);

								~PixelFormat();


			void				SetDrawingMode(drawing_mode mode,
											   source_alpha alphaSrcMode,
											   alpha_function alphaFncMode,
											   bool text);

			// AGG "pixel format" interface
	inline	unsigned			width() const
									{ return fBuffer->width(); }
	inline	unsigned			height() const
									{ return fBuffer->height(); }
	inline	int					stride() const
									{ return fBuffer->stride(); }

	inline	uint8*				row_ptr(int y)
									{ return fBuffer->row_ptr(y); }
	inline	const uint8*		row_ptr(int y) const
									{ return fBuffer->row_ptr(y); }
	inline	row_data			row(int y) const
									{ return fBuffer->row(y); }

	inline	uint8*				pix_ptr(int x, int y);
	inline	const uint8*		pix_ptr(int x, int y) const;

	inline	static	void		make_pix(uint8* p, const color_type& c);
//	inline	color_type			pixel(int x, int y) const;

	inline	void				blend_pixel(int x, int y,
											const color_type& c,
											uint8 cover);


	inline	void				blend_hline(int x, int y,
											unsigned len,
											const color_type& c,
											uint8 cover);

	inline	void				blend_vline(int x, int y,
											unsigned len,
											const color_type& c,
											uint8 cover);

	inline	void				blend_solid_hspan(int x, int y,
												  unsigned len,
												  const color_type& c,
												  const uint8* covers);

	inline	void				blend_solid_hspan_subpix(int x, int y,
												  unsigned len,
												  const color_type& c,
												  const uint8* covers);

	inline	void				blend_solid_vspan(int x, int y,
												  unsigned len,
												  const color_type& c,
												  const uint8* covers);

	inline	void				blend_color_hspan(int x, int y,
												  unsigned len,
												  const color_type* colors,
												  const uint8* covers,
												  uint8 cover);

	inline	void				blend_color_vspan(int x, int y,
												  unsigned len,
												  const color_type* colors,
												  const uint8* covers,
												  uint8 cover);

 private:
	agg::rendering_buffer*		fBuffer;
	const PatternHandler*		fPatternHandler;

	blend_pixel_f				fBlendPixel;
	blend_line					fBlendHLine;
	blend_line					fBlendVLine;
	blend_solid_span			fBlendSolidHSpan;
	blend_solid_span            fBlendSolidHSpanSubpix;
	blend_solid_span			fBlendSolidVSpan;
	blend_color_span			fBlendColorHSpan;
	blend_color_span			fBlendColorVSpan;

	template<typename T>
	void SetAggCompOpAdapter()
	{
		fBlendPixel = T::blend_pixel;
		fBlendHLine = T::blend_hline;
		fBlendSolidHSpanSubpix = T::blend_solid_hspan_subpix;
		fBlendSolidHSpan = T::blend_solid_hspan;
		fBlendSolidVSpan = T::blend_solid_vspan;
		fBlendColorHSpan = T::blend_color_hspan;
	}
};

// inlined functions

// pix_ptr
inline uint8*
PixelFormat::pix_ptr(int x, int y)
{
	return fBuffer->row_ptr(y) + x * pix_width;
}

// pix_ptr
inline const uint8*
PixelFormat::pix_ptr(int x, int y) const
{
	return fBuffer->row_ptr(y) + x * pix_width;
}

// make_pix
inline void
PixelFormat::make_pix(uint8* p, const color_type& c)
{
	p[0] = c.b;
	p[1] = c.g;
	p[2] = c.r;
	p[3] = c.a;
}

//// pixel
//inline color_type
//PixelFormat::pixel(int x, int y) const
//{
//	const uint8* p = (const uint8*)fBuffer->row_ptr(y);
//	if (p) {
//		p += x << 2;
//		return color_type(p[2], p[1], p[0], p[3]);
//	}
//	return color_type::no_color();
//}

// blend_pixel
inline void
PixelFormat::blend_pixel(int x, int y, const color_type& c, uint8 cover)
{
	fBlendPixel(x, y, c, cover, fBuffer, fPatternHandler);
}

// blend_hline
inline void
PixelFormat::blend_hline(int x, int y, unsigned len,
						 const color_type& c, uint8 cover)
{
	fBlendHLine(x, y, len, c, cover, fBuffer, fPatternHandler);
}

// blend_vline
inline void
PixelFormat::blend_vline(int x, int y, unsigned len,
						 const color_type& c, uint8 cover)
{
	fBlendVLine(x, y, len, c, cover, fBuffer, fPatternHandler);
}

// blend_solid_hspan
inline void
PixelFormat::blend_solid_hspan(int x, int y, unsigned len,
							   const color_type& c, const uint8* covers)
{
	fBlendSolidHSpan(x, y, len, c, covers, fBuffer, fPatternHandler);
}

// blend_solid_hspan_subpix
inline void
PixelFormat::blend_solid_hspan_subpix(int x, int y, unsigned len,
							   const color_type& c, const uint8* covers)
{
	fBlendSolidHSpanSubpix(x, y, len, c, covers, fBuffer, fPatternHandler);
}

// blend_solid_vspan
inline void
PixelFormat::blend_solid_vspan(int x, int y, unsigned len,
							   const color_type& c, const uint8* covers)
{
	fBlendSolidVSpan(x, y, len, c, covers, fBuffer, fPatternHandler);
}

// blend_color_hspan
inline void
PixelFormat::blend_color_hspan(int x, int y, unsigned len,
							   const color_type* colors,
							   const uint8* covers,
							   uint8 cover)
{
	fBlendColorHSpan(x, y, len, colors, covers, cover,
					 fBuffer, fPatternHandler);
}

// blend_color_vspan
inline void
PixelFormat::blend_color_vspan(int x, int y, unsigned len,
							   const color_type* colors,
							   const uint8* covers,
							   uint8 cover)
{
	fBlendColorVSpan(x, y, len, colors, covers, cover,
					 fBuffer, fPatternHandler);
}

#endif // PIXEL_FORMAT_H

