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
	// This class needs to be defined before any inline functions, as otherwise
	// gcc2 will barf in debug mode.
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
		float advanceX, float advanceY, float preciseAdvanceX,
		float preciseAdvanceY, float insetLeft, float insetRight)
	{
		GlyphCache* glyph = fGlyphTable.Lookup(glyphIndex);
		if (glyph != NULL)
			return NULL;

		glyph = new(std::nothrow) GlyphCache(glyphIndex, dataSize, dataType,
			bounds, advanceX, advanceY, preciseAdvanceX, preciseAdvanceY,
			insetLeft, insetRight);
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
}


bool
FontCacheEntry::Init(const ServerFont& font, bool forceVector)
{
	if (fGlyphCache.Get() == NULL)
		return false;

	glyph_rendering renderingType = _RenderTypeFor(font, forceVector);

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
	uint32 glyphCode;
	const char* start = utf8String;
	while ((glyphCode = UTF8ToCharCode(&utf8String))) {
		if (fGlyphCache->FindGlyph(glyphCode) == NULL)
			return false;
		if (utf8String - start + 1 > length)
			break;
	}
	return true;
}


inline bool
render_as_space(uint32 glyphCode)
{
	// whitespace: render as space
	// as per Unicode PropList.txt: White_Space
	return (glyphCode >= 0x0009 && glyphCode <= 0x000d)
			// control characters
		|| (glyphCode == 0x0085)
			// another control
		|| (glyphCode == 0x00a0)
			// no-break space
		|| (glyphCode == 0x1680)
			// ogham space mark
		|| (glyphCode == 0x180e)
			// mongolian vowel separator
		|| (glyphCode >= 0x2000 && glyphCode <= 0x200a)
			// en quand, hair space
		|| (glyphCode >= 0x2028 && glyphCode <= 0x2029)
			// line and paragraph separators
		|| (glyphCode == 0x202f)
			// narrow no-break space
		|| (glyphCode == 0x205f)
			// medium math space
		|| (glyphCode == 0x3000)
			// ideographic space
		;
}


inline bool
render_as_zero_width(uint32 glyphCode)
{
	// ignorable chars: render as invisible
	// as per Unicode DerivedCoreProperties.txt: Default_Ignorable_Code_Point
	return (glyphCode == 0x00ad)
			// soft hyphen
		|| (glyphCode == 0x034f)
			// combining grapheme joiner
		|| (glyphCode >= 0x115f && glyphCode <= 0x1160)
			// hangul fillers
		|| (glyphCode >= 0x17b4 && glyphCode <= 0x17b5)
			// ignorable khmer vowels
		|| (glyphCode >= 0x180b && glyphCode <= 0x180d)
			// variation selectors
		|| (glyphCode >= 0x200b && glyphCode <= 0x200f)
			// zero width space, cursive joiners, ltr marks
		|| (glyphCode >= 0x202a && glyphCode <= 0x202e)
			// left to right embed, override
		|| (glyphCode >= 0x2060 && glyphCode <= 0x206f)
			// word joiner, invisible math operators, reserved
		|| (glyphCode == 0x3164)
			// hangul filler
		|| (glyphCode >= 0xfe00 && glyphCode <= 0xfe0f)
			// variation selectors
		|| (glyphCode == 0xfeff)
			// zero width no-break space
		|| (glyphCode == 0xffa0)
			// halfwidth hangul filler
		|| (glyphCode >= 0xfff0 && glyphCode <= 0xfff8)
			// reserved
		|| (glyphCode >= 0x1d173 && glyphCode <= 0x1d17a)
			// musical symbols
		|| (glyphCode >= 0xe0000 && glyphCode <= 0xe01ef)
			// variation selectors, tag space, reserved
		;
}


const GlyphCache*
FontCacheEntry::CachedGlyph(uint32 glyphCode)
{
	// Only requires a read lock.
	return fGlyphCache->FindGlyph(glyphCode);
}


bool
FontCacheEntry::CanCreateGlyph(uint32 glyphCode)
{
	// Note that this bypass any fallback or caching because it is used in
	// the fallback code itself.
	uint32 glyphIndex = fEngine.GlyphIndexForGlyphCode(glyphCode);
	return glyphIndex != 0;
}


const GlyphCache*
FontCacheEntry::CreateGlyph(uint32 glyphCode, FontCacheEntry* fallbackEntry)
{
	// We cache the glyph by the requested glyphCode. The FontEngine of this
	// FontCacheEntry may not contain a glyph for the given code, in which case
	// we ask the fallbackEntry for the code to index translation and let it
	// generate the glyph data. We will still use our own cache for storing the
	// glyph. The next time it will be found (by glyphCode).

	// NOTE: Both this and the fallback FontCacheEntry are expected to be
	// write-locked!

	const GlyphCache* glyph = fGlyphCache->FindGlyph(glyphCode);
	if (glyph != NULL)
		return glyph;

	FontEngine* engine = &fEngine;
	uint32 glyphIndex = engine->GlyphIndexForGlyphCode(glyphCode);
	if (glyphIndex == 0 && fallbackEntry != NULL) {
		// Our FontEngine does not contain this glyph, but we can retry with
		// the fallbackEntry.
		engine = &fallbackEntry->fEngine;
		glyphIndex = engine->GlyphIndexForGlyphCode(glyphCode);
	}

	if (glyphIndex == 0) {
		if (render_as_zero_width(glyphCode)) {
			// cache and return a zero width glyph
			return fGlyphCache->CacheGlyph(glyphCode, 0, glyph_data_invalid,
				agg::rect_i(0, 0, -1, -1), 0, 0, 0, 0, 0, 0);
		}

		// reset to our engine
		engine = &fEngine;
		if (render_as_space(glyphCode)) {
			// get the normal space glyph
			glyphIndex = engine->GlyphIndexForGlyphCode(0x20 /* space */);
		} else {
			// The glyph was not found anywhere.
			return NULL;
		}
	}

	if (engine->PrepareGlyph(glyphIndex)) {
		glyph = fGlyphCache->CacheGlyph(glyphCode,
			engine->DataSize(), engine->DataType(), engine->Bounds(),
			engine->AdvanceX(), engine->AdvanceY(),
			engine->PreciseAdvanceX(), engine->PreciseAdvanceY(),
			engine->InsetLeft(), engine->InsetRight());

		if (glyph != NULL)
			engine->WriteGlyphTo(glyph->data);
	}

	return glyph;
}


void
FontCacheEntry::InitAdaptors(const GlyphCache* glyph,
	double x, double y, GlyphMonoAdapter& monoAdapter,
	GlyphGray8Adapter& gray8Adapter, GlyphPathAdapter& pathAdapter,
	double scale)
{
	if (glyph == NULL)
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
	const ServerFont& font, bool forceVector)
{
	glyph_rendering renderingType = _RenderTypeFor(font, forceVector);

	// TODO: read more of these from the font
	FT_Encoding charMap = FT_ENCODING_NONE;
	bool hinting = font.Hinting();
	uint8 averageWeight = gSubpixelAverageWeight;

	snprintf(signature, signatureSize, "%" B_PRId32 ",%u,%d,%d,%.1f,%d,%d",
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
FontCacheEntry::_RenderTypeFor(const ServerFont& font, bool forceVector)
{
	glyph_rendering renderingType = gSubpixelAntialiasing ?
		glyph_ren_subpix : glyph_ren_native_gray8;

	if (forceVector || font.Rotation() != 0.0 || font.Shear() != 90.0
		|| font.FalseBoldWidth() != 0.0
		|| (font.Flags() & B_DISABLE_ANTIALIASING) != 0
		|| font.Size() > 30
		|| !font.Hinting()) {
		renderingType = glyph_ren_outline;
	}

	return renderingType;
}
