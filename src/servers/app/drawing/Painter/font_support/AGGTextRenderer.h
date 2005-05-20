// AGGTextRenderer.h

#ifndef AGG_TEXT_RENDERER_H
#define AGG_TEXT_RENDERER_H

#include <agg_conv_curve.h>
#include <agg_conv_contour.h>
#include <agg_scanline_u.h>

#include "agg_font_freetype.h"
#include "defines.h"

#include "Transformable.h"

class ServerFont;

class AGGTextRenderer {
 public:
								AGGTextRenderer();
	virtual						~AGGTextRenderer();

			bool				SetFont(const ServerFont &font);
			void				Unset();

			void				SetPointSize(float size);

			void				SetHinting(bool hinting);
			bool				Hinting() const
									{ return fHinted; }

			void				SetAntialiasing(bool antialiasing);
			bool				Antialiasing() const
									{ return fAntialias; }

			BRect				RenderString(const char* utf8String,
											 uint32 length,
											 font_renderer_solid_type* solidRenderer,
											 font_renderer_bin_type* binRenderer,
											 const Transformable& transform,
											 BRect clippingFrame,
											 bool dryRun = false,
											 BPoint* nextCharPos = NULL);

			double				StringWidth(const char* utf8String,
											uint32 length);

 private:
			status_t			_PrepareUnicodeBuffer(const char* utf8String,
													  uint32 length,
													  uint32* glyphCount);


	typedef agg::font_engine_freetype_int32				font_engine_type;
	typedef agg::font_cache_manager<font_engine_type>	font_manager_type;
	typedef agg::conv_curve<font_manager_type::path_adaptor_type>
														conv_font_curve_type;
	typedef agg::conv_contour<conv_font_curve_type>		conv_font_contour_type;

	font_engine_type			fFontEngine;
	font_manager_type			fFontManager;

	// Pipeline to process the vectors glyph paths (curves + contour)
	conv_font_curve_type		fCurves;
	conv_font_contour_type		fContour;

	agg::scanline_u8			fScanline;
	agg::rasterizer_scanline_aa<>	fRasterizer;

	char*						fUnicodeBuffer;
	int32						fUnicodeBufferSize;

	bool						fHinted;		// is glyph hinting active?
	bool						fAntialias;		// is anti-aliasing active?
	bool						fKerning;
};

#endif // AGG_TEXT_RENDERER_H
