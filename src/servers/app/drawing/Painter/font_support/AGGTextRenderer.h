/*
 * Copyright 2005-2006, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef AGG_TEXT_RENDERER_H
#define AGG_TEXT_RENDERER_H

#include "agg_font_freetype.h"
#include "defines.h"

#include "ServerFont.h"
#include "Transformable.h"

#include <agg_conv_curve.h>
#include <agg_conv_contour.h>
#include <agg_scanline_u.h>


class AGGTextRenderer {
 public:
								AGGTextRenderer();
	virtual						~AGGTextRenderer();

			bool				SetFont(const ServerFont &font);
			void				Unset();

			void				SetHinting(bool hinting);
			bool				Hinting() const
									{ return fHinted; }

			void				SetAntialiasing(bool antialiasing);
			bool				Antialiasing() const
									{ return fAntialias; }

			void				SetKerning(bool kerning);
			bool				Kerning() const
									{ return fKerning; }

			BRect				RenderString(const char* utf8String,
											 uint32 length,
											 font_renderer_solid_type* solidRenderer,
											 font_renderer_bin_type* binRenderer,
											 const BPoint& baseLine,
											 const BRect& clippingFrame,
											 bool dryRun = false,
											 BPoint* nextCharPos = NULL,
											 const escapement_delta* delta = NULL);

			double				StringWidth(const char* utf8String,
											uint32 length);

 private:
			void				_UpdateSizeAndHinting(float size, bool hinted,
													  bool force = false);


	typedef agg::font_engine_freetype_int32				font_engine_type;
	typedef agg::font_cache_manager<font_engine_type>	font_cache_type;
	typedef agg::conv_curve<font_cache_type::path_adaptor_type>
														conv_font_curve_type;
	typedef agg::conv_contour<conv_font_curve_type>		conv_font_contour_type;

	font_engine_type			fFontEngine;
	font_cache_type				fFontCache;

	// Pipeline to process the vectors glyph paths (curves + contour)
	conv_font_curve_type		fCurves;
	conv_font_contour_type		fContour;

	agg::scanline_u8			fScanline;
	agg::rasterizer_scanline_aa<>	fRasterizer;

	char*						fUnicodeBuffer;
	int32						fUnicodeBufferSize;

	bool						fHinted;		// is glyph hinting active?
	bool						fAntialias;
	bool						fKerning;
	Transformable				fEmbeddedTransformation;	// rotated or sheared font?

	// caching to avoid loading a font unnecessarily
	ServerFont					fFont;
	uint32						fLastFamilyAndStyle;
	float						fLastPointSize;
	bool						fLastHinted;
};

#endif // AGG_TEXT_RENDERER_H
