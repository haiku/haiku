/*
 * Copyright 2005-2006, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef AGG_TEXT_RENDERER_H
#define AGG_TEXT_RENDERER_H

#include "defines.h"

#include "FontCacheEntry.h"
#include "ServerFont.h"
#include "Transformable.h"

#include <agg_conv_curve.h>
#include <agg_conv_contour.h>
#include <agg_scanline_u.h>


class AGGTextRenderer {
 public:
								AGGTextRenderer();
	virtual						~AGGTextRenderer();

			void				SetFont(const ServerFont &font);

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
											 renderer_type* solidRenderer,
											 renderer_bin_type* binRenderer,
											 scanline_unpacked_type& scanline,
											 const BPoint& baseLine,
											 const BRect& clippingFrame,
											 bool dryRun = false,
											 BPoint* nextCharPos = NULL,
											 const escapement_delta* delta = NULL);

 private:

	class StringRenderer;
	friend class StringRenderer;

	// Pipeline to process the vectors glyph paths (curves + contour)
	FontCacheEntry::GlyphPathAdapter	fPathAdaptor;
	FontCacheEntry::GlyphGray8Adapter	fGray8Adaptor;
	FontCacheEntry::GlyphGray8Scanline	fGray8Scanline;
	FontCacheEntry::GlyphMonoAdapter	fMonoAdaptor;
	FontCacheEntry::GlyphMonoScanline	fMonoScanline;

	FontCacheEntry::CurveConverter		fCurves;
	FontCacheEntry::ContourConverter	fContour;

	rasterizer_type				fRasterizer;
		// NOTE: the object has it's own rasterizer object
		// since it might be using a different gamma setting
		// to support non-anti-aliased text rendering

	ServerFont					fFont;
	bool						fHinted;		// is glyph hinting active?
	bool						fAntialias;
	bool						fKerning;
	Transformable				fEmbeddedTransformation;	// rotated or sheared font?

	static	AGGTextRenderer		sDefaultInstance;
};

#endif // AGG_TEXT_RENDERER_H
