/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
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
#include <agg_color_rgba8.h>
#include <agg_rendering_buffer.h>

#include <GraphicsDefs.h>

class PatternHandler;

class PixelFormat {
 public:
	typedef agg::rgba8						color_type;
	typedef agg::rendering_buffer			agg_buffer;

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
			unsigned			width()  const { return fBuffer->width();  }
			unsigned			height() const { return fBuffer->height(); }

			void				blend_pixel(int x, int y,
											const color_type& c,
											uint8 cover);


			void				blend_hline(int x, int y,
											unsigned len,
											const color_type& c,
											uint8 cover);

			void				blend_vline(int x, int y,
											unsigned len,
											const color_type& c,
											uint8 cover);

			void				blend_solid_hspan(int x, int y,
												  unsigned len,
												  const color_type& c,
												  const uint8* covers);

			void				blend_solid_vspan(int x, int y,
												  unsigned len,
												  const color_type& c,
												  const uint8* covers);

			void				blend_color_hspan(int x, int y,
												  unsigned len,
												  const color_type* colors,
												  const uint8* covers,
												  uint8 cover);

			void				blend_color_vspan(int x, int y,
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
	blend_solid_span			fBlendSolidVSpan;
	blend_color_span			fBlendColorHSpan;
	blend_color_span			fBlendColorVSpan;
};

// inlined functions

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

