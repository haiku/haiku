/*
 * Copyright 2007-2009, Haiku. All rights reserved.
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

#include <new>

#include <Autolock.h>

#include <agg_array.h>
#include <utf8_functions.h>
#include <util/OpenHashTable.h>

#include "GlobalSubpixelSettings.h"


BLocker FontCacheEntry::sUsageUpdateLock("FontCacheEntry usage lock");


class FontCacheEntry::GlyphCachePool {
public:
	GlyphCachePool()
	{
	}

	~GlyphCachePool()
	{
		GlyphCache* glyph = fGlyphTable.Clear(true);
		while (glyph != NULL) {
			GlyphCache* next = glyph->hash_link;
			delete glyph;
			glyph = next;
		}
	}

	status_t Init()
	{
		return fGlyphTable.Init();
	}

	const GlyphCache* FindGlyph(uint32 glyphIndex) const
	{
		return fGlyphTable.Lookup(glyphIndex);
	}

	GlyphCache* CacheGlyph(uint32 glyphIndex,
		uint32 dataSize, glyph_data_type dataType, const agg::rect_i& bounds,
		float advanceX, float advanceY, float insetLeft, float insetRight)
	{
		GlyphCache* glyph = fGlyphTable.Lookup(glyphIndex);
		if (glyph != NULL)
			return NULL;

		glyph = new(std::nothrow) GlyphCache(glyphIndex, dataSize, dataType,
			bounds, advanceX, advanceY, insetLeft, insetRight);
		if (glyph == NULL || glyph->data == NULL) {
			delete glyph;
			return NULL;
		}

		// TODO: The HashTable grows without bounds. We should cleanup
		// older entries from time to time.

		fGlyphTable.Insert(glyph);

		return glyph;
	}

private:
	struct GlyphHashTableDefinition {
		typedef uint32		KeyType;
		typedef	GlyphCache	ValueType;

		size_t HashKey(uint32 key) const
		{
			return key;
		}

		size_t Hash(GlyphCache* value) const
		{
			return value->glyph_index;
		}

		bool Compare(uint32 key, GlyphCache* value) const
		{
			return value->glyph_index == key;
		}

		GlyphCache*& GetLink(GlyphCache* value) const
		{
			return value->hash_link;
		}
	};

	typedef BOpenHashTable<GlyphHashTableDefinition> GlyphTable;

	GlyphTable	fGlyphTable;
};


// #pragma mark -


FontCacheEntry::FontCacheEntry()
	:
	MultiLocker("FontCacheEntry lock"),
	fGlyphCache(new(std::nothrow) GlyphCachePool()),
	fEngine(),
	fLastUsedTime(LONGLONG_MIN),
	fUseCounter(0)
{
}


FontCacheEntry::~FontCacheEntry()
{
//printf("~FontCacheEntry()\n");
	delete fGlyphCache;
}


bool
FontCacheEntry::Init(const ServerFont& font)
{
	if (fGlyphCache == NULL)
		return false;

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
	if (fGlyphCache->Init() != B_OK) {
		fprintf(stderr, "FontCacheEntry::Init() - failed to allocate "
			"GlyphCache table for font file %s\n", font.Path());
		return false;
	}

	return true;
}


bool
FontCacheEntry::HasGlyphs(const char* utf8String, ssize_t length) const
{
	uint32 charCode;
	const char* start = utf8String;
	while ((charCode = UTF8ToCharCode(&utf8String))) {
		uint32 glyphIndex = fEngine.GlyphIndexForGlyphCode(charCode);
		if (!fGlyphCache->FindGlyph(glyphIndex))
			return false;
		if (utf8String - start + 1 > length)
			break;
	}
	return true;
}


const GlyphCache*
FontCacheEntry::Glyph(uint32 glyphCode)
{
	uint32 glyphIndex = fEngine.GlyphIndexForGlyphCode(glyphCode);
	if (glyphIndex==0)
		return NULL;
	const GlyphCache* glyph = fGlyphCache->FindGlyph(glyphIndex);
	if (glyph) {
		return glyph;
	} else {
		if (fEngine.PrepareGlyph(glyphIndex)) {
			glyph = fGlyphCache->CacheGlyph(glyphIndex,
				fEngine.DataSize(), fEngine.DataType(), fEngine.Bounds(),
				fEngine.AdvanceX(), fEngine.AdvanceY(),
				fEngine.InsetLeft(), fEngine.InsetRight());

			if (glyph != NULL)
				fEngine.WriteGlyphTo(glyph->data);

			return glyph;
		}
	}
	return NULL;
}


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


bool
FontCacheEntry::GetKerning(uint32 glyphCode1, uint32 glyphCode2,
	double* x, double* y)
{
	return fEngine.GetKerning(glyphCode1, glyphCode2, x, y);
}


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
