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


#include "FontCacheEntry.h"

#include <string.h>
#include <agg_array.h>

#include <Autolock.h>

#include "utf8_functions.h"

#include "GlobalSubpixelSettings.h"


BLocker
FontCacheEntry::sUsageUpdateLock("FontCacheEntry usage lock");

class FontCacheEntry::GlyphCachePool {
 public:
	enum block_size_e { block_size = 16384-16 };

	GlyphCachePool()
		: fAllocator(block_size)
	{
		memset(fGlyphs, 0, sizeof(fGlyphs));
	}

	const GlyphCache* FindGlyph(uint16 glyphCode) const
	{
		unsigned msb = (glyphCode >> 8) & 0xFF;
		if (fGlyphs[msb])
			return fGlyphs[msb][glyphCode & 0xFF];
		return 0;
	}

	GlyphCache* CacheGlyph(uint16 glyphCode, unsigned glyphIndex,
		unsigned dataSize, glyph_data_type dataType, const agg::rect_i& bounds,
		float advanceX, float advanceY, float insetLeft, float insetRight)
	{
		unsigned msb = (glyphCode >> 8) & 0xFF;
		if (fGlyphs[msb] == 0) {
			fGlyphs[msb]
				= (GlyphCache**)fAllocator.allocate(sizeof(GlyphCache*) * 256,
						sizeof(GlyphCache*));
			memset(fGlyphs[msb], 0, sizeof(GlyphCache*) * 256);
		}

		unsigned lsb = glyphCode & 0xFF;
		if (fGlyphs[msb][lsb])
			return NULL; // already exists, do not overwrite

		GlyphCache* glyph
			= (GlyphCache*)fAllocator.allocate(sizeof(GlyphCache),
					sizeof(double));

		if (glyph == NULL)
			return NULL;

		glyph->glyph_index = glyphIndex;
		glyph->data = fAllocator.allocate(dataSize);
		glyph->data_size = dataSize;
		glyph->data_type = dataType;
		glyph->bounds = bounds;
		glyph->advance_x = advanceX;
		glyph->advance_y = advanceY;
		glyph->inset_left = insetLeft;
		glyph->inset_right = insetRight;

		return fGlyphs[msb][lsb] = glyph;
	}

 private:
	agg::block_allocator	fAllocator;
	GlyphCache**			fGlyphs[256];
};

// #pragma mark -

// constructor
FontCacheEntry::FontCacheEntry()
	: MultiLocker("FontCacheEntry lock")
	, Referenceable()
	, fGlyphCache(new GlyphCachePool())
	, fEngine()
	, fLastUsedTime(LONGLONG_MIN)
	, fUseCounter(0)
{
}

// destructor
FontCacheEntry::~FontCacheEntry()
{
//printf("~FontCacheEntry()\n");
	delete fGlyphCache;
}

// Init
bool
FontCacheEntry::Init(const ServerFont& font)
{
	glyph_rendering renderingType = _RenderTypeFor(font);

	// TODO: encoding from font
	FT_Encoding charMap = FT_ENCODING_NONE;
	bool hinting = font.Hinting();

	if (!fEngine.Init(font.Path(), 0, font.Size(), charMap,
			renderingType, hinting)) {
		fprintf(stderr, "FontCacheEntry::Init() - some error loading font "
			"file %s\n", font.Path());
		return false;
	}
	return true;
}

// HasGlyphs
bool
FontCacheEntry::HasGlyphs(const char* utf8String, ssize_t length) const
{
	uint32 charCode;
	const char* start = utf8String;
	while ((charCode = UTF8ToCharCode(&utf8String))) {
		if (!fGlyphCache->FindGlyph(charCode))
			return false;
		if (utf8String - start + 1 > length)
			break;
	}
	return true;
}

// Glyph
const GlyphCache*
FontCacheEntry::Glyph(uint16 glyphCode)
{
	const GlyphCache* glyph = fGlyphCache->FindGlyph(glyphCode);
	if (glyph) {
		return glyph;
	} else {
		if (fEngine.PrepareGlyph(glyphCode)) {
			glyph = fGlyphCache->CacheGlyph(glyphCode,
				fEngine.GlyphIndex(), fEngine.DataSize(),
				fEngine.DataType(), fEngine.Bounds(),
				fEngine.AdvanceX(), fEngine.AdvanceY(),
				fEngine.InsetLeft(), fEngine.InsetRight());

			fEngine.WriteGlyphTo(glyph->data);

			return glyph;
		}
	}
	return 0;
}

// InitAdaptors
void
FontCacheEntry::InitAdaptors(const GlyphCache* glyph,
	double x, double y, GlyphMonoAdapter& monoAdapter,
	GlyphGray8Adapter& gray8Adapter, GlyphPathAdapter& pathAdapter,
	double scale)
{
	if (!glyph)
		return;

	switch(glyph->data_type) {
		case glyph_data_mono:
			monoAdapter.init(glyph->data, glyph->data_size, x, y);
			break;

		case glyph_data_gray8:
			gray8Adapter.init(glyph->data, glyph->data_size, x, y);
			break;

		case glyph_data_subpix:
			gray8Adapter.init(glyph->data, glyph->data_size, x, y);
			break;

		case glyph_data_outline:
			pathAdapter.init(glyph->data, glyph->data_size, x, y, scale);
			break;

		default:
			break;
	}
}

// GetKerning
bool
FontCacheEntry::GetKerning(uint16 glyphCode1, uint16 glyphCode2,
	double* x, double* y)
{
	return fEngine.GetKerning(glyphCode1, glyphCode2, x, y);
}

// GenerateSignature
/*static*/ void
FontCacheEntry::GenerateSignature(char* signature, size_t signatureSize,
	const ServerFont& font)
{
	glyph_rendering renderingType = _RenderTypeFor(font);

	// TODO: read more of these from the font
	FT_Encoding charMap = FT_ENCODING_NONE;
	bool hinting = font.Hinting();
	uint8 averageWeight = gSubpixelAverageWeight;

	snprintf(signature, signatureSize, "%ld,%u,%d,%d,%.1f,%d,%d",
		font.GetFamilyAndStyle(), charMap,
		font.Face(), int(renderingType), font.Size(), hinting, averageWeight);
}

// UpdateUsage
void
FontCacheEntry::UpdateUsage()
{
	// this is a static lock to prevent usage of too many semaphores,
	// but on the other hand, it is not so nice to be using a lock
	// here at all
	// the hope is that the time is so short to hold this lock, that
	// there is not much contention
	BAutolock _(sUsageUpdateLock);

	fLastUsedTime = system_time();
	fUseCounter++;
}


// _RenderTypeFor
/*static*/ glyph_rendering
FontCacheEntry::_RenderTypeFor(const ServerFont& font)
{
	glyph_rendering renderingType = gSubpixelAntialiasing ?
		glyph_ren_subpix : glyph_ren_native_gray8;

	if (font.Rotation() != 0.0 || font.Shear() != 90.0
		|| font.FalseBoldWidth() != 0.0
		|| font.Flags() & B_DISABLE_ANTIALIASING
		|| font.Size() > 30
		|| !font.Hinting()) {
		renderingType = glyph_ren_outline;
	}

	return renderingType;
}
