/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Maxim Shemanarev <mcseemagg@yahoo.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 */

//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
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

#ifndef FONT_ENGINE_H
#define FONT_ENGINE_H

#include <SupportDefs.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <agg_scanline_storage_aa.h>
#include <agg_scanline_storage_bin.h>
#include <agg_scanline_u.h>
#include <agg_scanline_bin.h>
#include <agg_path_storage_integer.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_conv_curve.h>
#include <agg_trans_affine.h>

#include "agg_scanline_storage_subpix.h"
#include "agg_scanline_u_subpix.h"
#include "agg_scanline_u_subpix_avrg_filtering.h"

#include "GlobalSubpixelSettings.h"


enum glyph_rendering {
	glyph_ren_native_mono,
	glyph_ren_native_gray8,
	glyph_ren_outline,
	glyph_ren_subpix
};


enum glyph_data_type {
	glyph_data_invalid	= 0,
	glyph_data_mono		= 1,
	glyph_data_gray8	= 2,
	glyph_data_outline	= 3,
	glyph_data_subpix   = 4
};


class FontEngine {
 public:
	typedef agg::serialized_scanlines_adaptor_subpix<uint8>	SubpixAdapter;
	typedef agg::serialized_scanlines_adaptor_aa<uint8>		Gray8Adapter;
	typedef agg::serialized_scanlines_adaptor_bin			MonoAdapter;
	typedef agg::scanline_storage_aa8						ScanlineStorageAA;
	typedef agg::scanline_storage_subpix8					ScanlineStorageSubpix;
	typedef agg::scanline_storage_bin						ScanlineStorageBin;
	typedef agg::serialized_integer_path_adaptor<int32, 6>	PathAdapter;

								FontEngine();
	virtual						~FontEngine();

			bool				Init(const char* fontFilePath,
									unsigned face_index, double size,
									FT_Encoding char_map,
									glyph_rendering ren_type,
									bool hinting,
									const char* fontFileBuffer = NULL,
									const long fontFileBufferSize = 0);

			int					LastError() const
									{ return fLastError; }
			unsigned			CountFaces() const;
			bool				Hinting() const
									{ return fHinting; }


			uint32				GlyphIndexForGlyphCode(uint32 glyphCode) const;
			bool				PrepareGlyph(uint32 glyphIndex);

			uint32				DataSize() const
									{ return fDataSize; }
			glyph_data_type		DataType() const
									{ return fDataType; }
			const agg::rect_i&	Bounds() const
									{ return fBounds; }
			double				AdvanceX() const
									{ return fAdvanceX; }
			double				AdvanceY() const
									{ return fAdvanceY; }
			double				InsetLeft() const
									{ return fInsetLeft; }
			double				InsetRight() const
									{ return fInsetRight; }

			void				WriteGlyphTo(uint8* data) const;


			bool				GetKerning(uint32 first, uint32 second,
									double* x, double* y);

 private:
			// disallowed stuff:
								FontEngine(const FontEngine&);
			const FontEngine&	operator=(const FontEngine&);

			int					fLastError;
			bool				fLibraryInitialized;
			FT_Library			fLibrary;	// handle to library
			FT_Face				fFace;	  // FreeType font face handle

			glyph_rendering		fGlyphRendering;
			bool				fHinting;

			// members needed to generate individual glyphs according
			// to glyph rendering type
			uint32				fDataSize;
			glyph_data_type		fDataType;
			agg::rect_i			fBounds;
			double				fAdvanceX;
			double				fAdvanceY;
			double				fInsetLeft;
			double				fInsetRight;

			// these members are for caching memory allocations
			// when rendering glyphs
	typedef agg::path_storage_integer<int32, 6>		PathStorageType;
	typedef agg::conv_curve<PathStorageType>		CurveConverterType;

			PathStorageType		fPath;
			CurveConverterType	fCurves;
			agg::scanline_u8	fScanlineAA;
			agg::scanline_bin	fScanlineBin;
#ifdef AVERAGE_BASED_SUBPIXEL_FILTERING
			agg::scanline_u8_subpix_avrg_filtering fScanlineSubpix;
#else
			agg::scanline_u8_subpix fScanlineSubpix;
#endif

			ScanlineStorageAA	fScanlineStorageAA;
			ScanlineStorageBin	fScanlineStorageBin;
			ScanlineStorageSubpix fScanlineStorageSubpix;
};


#endif // FONT_ENGINE_H
