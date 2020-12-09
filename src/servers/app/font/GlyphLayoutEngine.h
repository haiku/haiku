/*
 * Copyright 2007-2014, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef GLYPH_LAYOUT_ENGINE_H
#define GLYPH_LAYOUT_ENGINE_H

#include "utf8_functions.h"

#include "FontCache.h"
#include "FontCacheEntry.h"
#include "FontManager.h"
#include "ServerFont.h"

#include <Autolock.h>
#include <Debug.h>
#include <ObjectList.h>
#include <SupportDefs.h>

#include <ctype.h>

class FontCacheReference {
public:
	FontCacheReference()
		:
		fCacheEntry(NULL),
		fWriteLocked(false)
	{
	}

	~FontCacheReference()
	{
		Unset();
	}

	bool SetTo(FontCacheEntry* entry, bool writeLock)
	{
		ASSERT(entry != NULL);

		if (entry == fCacheEntry) {
			if (writeLock == fWriteLocked)
				return true;
			UnlockAndDisown();
		} else if (fCacheEntry != NULL)
			Unset();

		if (writeLock) {
			if (!entry->WriteLock()) {
				FontCache::Default()->Recycle(entry);
				return false;
			}
		} else if (!entry->ReadLock()) {
			FontCache::Default()->Recycle(entry);
			return false;
		}

		fCacheEntry = entry;
		fWriteLocked = writeLock;
		return true;
	}

	FontCacheEntry* UnlockAndDisown()
	{
		if (fCacheEntry == NULL)
			return NULL;

		if (fWriteLocked)
			fCacheEntry->WriteUnlock();
		else
			fCacheEntry->ReadUnlock();

		FontCacheEntry* entry = fCacheEntry;
		fCacheEntry = NULL;
		fWriteLocked = false;
		return entry;
	}

	void Unset()
	{
		if (fCacheEntry == NULL)
			return;

		FontCache::Default()->Recycle(UnlockAndDisown());
	}

	inline FontCacheEntry* Entry() const
	{
		return fCacheEntry;
	}

	inline bool WriteLocked() const
	{
		return fWriteLocked;
	}

private:
			FontCacheEntry*		fCacheEntry;
			bool				fWriteLocked;
};


class GlyphLayoutEngine {
public:
	static	bool				IsWhiteSpace(uint32 glyphCode);

	static	FontCacheEntry*		FontCacheEntryFor(const ServerFont& font,
									bool forceVector);

			template<class GlyphConsumer>
	static	bool				LayoutGlyphs(GlyphConsumer& consumer,
									const ServerFont& font,
									const char* utf8String,
									int32 length, int32 maxChars,
									const escapement_delta* delta = NULL,
									uint8 spacing = B_BITMAP_SPACING,
									const BPoint* offsets = NULL,
									FontCacheReference* cacheReference = NULL);

private:
	static	const GlyphCache*	_CreateGlyph(
									FontCacheReference& cacheReference,
									BObjectList<FontCacheReference>& fallbacks,
									const ServerFont& font, bool needsVector,
									uint32 glyphCode);

	static	void				_PopulateAndLockFallbacks(
									BObjectList<FontCacheReference>& fallbacks,
									const ServerFont& font, bool forceVector);

								GlyphLayoutEngine();
	virtual						~GlyphLayoutEngine();
};


inline bool
GlyphLayoutEngine::IsWhiteSpace(uint32 charCode)
{
	switch (charCode) {
		case 0x0009:	/* tab */
		case 0x000b:	/* vertical tab */
		case 0x000c:	/* form feed */
		case 0x0020:	/* space */
		case 0x00a0:	/* non breaking space */
		case 0x000a:	/* line feed */
		case 0x000d:	/* carriage return */
		case 0x2028:	/* line separator */
		case 0x2029:	/* paragraph separator */
			return true;
	}

	return false;
}


inline FontCacheEntry*
GlyphLayoutEngine::FontCacheEntryFor(const ServerFont& font, bool forceVector)
{
	FontCache* cache = FontCache::Default();
	FontCacheEntry* entry = cache->FontCacheEntryFor(font, forceVector);
	return entry;
}


template<class GlyphConsumer>
inline bool
GlyphLayoutEngine::LayoutGlyphs(GlyphConsumer& consumer,
	const ServerFont& font,
	const char* utf8String, int32 length, int32 maxChars,
	const escapement_delta* delta, uint8 spacing,
	const BPoint* offsets, FontCacheReference* _cacheReference)
{
	// TODO: implement spacing modes
	FontCacheEntry* entry = NULL;
	FontCacheReference* pCacheReference;
	FontCacheReference cacheReference;
	BObjectList<FontCacheReference> fallbacksList(21, true);

	if (_cacheReference != NULL) {
		pCacheReference = _cacheReference;
		entry = _cacheReference->Entry();
		// When there is already a cacheReference, it means there was already
		// an iteration over the glyphs. The use-case is for example to do
		// a layout pass to get the string width for the bounding box, then a
		// second layout pass to actually render the glyphs to the screen.
		// This means that the fallback entry mechanism will not do any good
		// for the second pass, since the fallback glyphs have been stored in
		// the original entry.
	} else
		pCacheReference = &cacheReference;

	if (entry == NULL) {
		entry = FontCacheEntryFor(font, consumer.NeedsVector());

		if (entry == NULL)
			return false;
		if (!pCacheReference->SetTo(entry, false))
			return false;
	} // else the entry was already used and is still locked

	consumer.Start();

	double x = 0.0;
	double y = 0.0;
	if (offsets) {
		x = offsets[0].x;
		y = offsets[0].y;
	}

	double advanceX = 0.0;
	double advanceY = 0.0;
	double size = font.Size();

	uint32 lastCharCode = 0; // Needed for kerning in B_STRING_SPACING mode
	uint32 charCode;
	int32 index = 0;
	const char* start = utf8String;
	while (maxChars-- > 0 && (charCode = UTF8ToCharCode(&utf8String)) != 0) {

		if (offsets != NULL) {
			// Use direct glyph locations instead of calculating them
			// from the advance values
			x = offsets[index].x;
			y = offsets[index].y;
		} else {
			if (spacing == B_STRING_SPACING)
				entry->GetKerning(lastCharCode, charCode, &advanceX, &advanceY);

			x += advanceX;
			y += advanceY;
		}

		const GlyphCache* glyph = entry->CachedGlyph(charCode);
		if (glyph == NULL) {
			glyph = _CreateGlyph(*pCacheReference, fallbacksList, font,
				consumer.NeedsVector(), charCode);

			// Something may have gone wrong while reacquiring the entry lock
			if (pCacheReference->Entry() == NULL)
				return false;
		}

		if (glyph == NULL) {
			consumer.ConsumeEmptyGlyph(index++, charCode, x, y);
			advanceX = 0;
			advanceY = 0;
		} else {
			// get next increment for pen position
			if (spacing == B_CHAR_SPACING) {
				advanceX = glyph->precise_advance_x * size;
				advanceY = glyph->precise_advance_y * size;
			} else {
				advanceX = glyph->advance_x;
				advanceY = glyph->advance_y;
			}

			// adjust for custom spacing
			if (delta != NULL) {
				advanceX += IsWhiteSpace(charCode)
					? delta->space : delta->nonspace;
			}

			if (!consumer.ConsumeGlyph(index++, charCode, glyph, entry, x, y,
					advanceX, advanceY)) {
				advanceX = 0.0;
				advanceY = 0.0;
				break;
			}
		}

		lastCharCode = charCode;
		if (utf8String - start + 1 > length)
			break;
	}

	x += advanceX;
	y += advanceY;
	consumer.Finish(x, y);

	return true;
}


inline const GlyphCache*
GlyphLayoutEngine::_CreateGlyph(FontCacheReference& cacheReference,
	BObjectList<FontCacheReference>& fallbacks,
	const ServerFont& font, bool forceVector, uint32 charCode)
{
	FontCacheEntry* entry = cacheReference.Entry();

	// Avoid loading and locking the fallbacks if our font can create the glyph.
	if (entry->CanCreateGlyph(charCode)) {
		if (cacheReference.SetTo(entry, true))
			return entry->CreateGlyph(charCode);
		return NULL;
	}

	if (fallbacks.IsEmpty()) {
		// We need to create new glyphs with the engine of the fallback font
		// and store them in the main font cache (not just transfer them from
		// one cache to the other). So we need both to be write-locked.
		// The main font is unlocked first, in case it is also in the fallback
		// list, so that we always keep the same order to avoid deadlocks.
		cacheReference.UnlockAndDisown();
		_PopulateAndLockFallbacks(fallbacks, font, forceVector);
		if (!cacheReference.SetTo(entry, true)) {
			return NULL;
		}
	}

	int32 count = fallbacks.CountItems();
	for (int32 index = 0; index < count; index++) {
		FontCacheEntry* fallbackEntry = fallbacks.ItemAt(index)->Entry();
		if (fallbackEntry->CanCreateGlyph(charCode))
			return entry->CreateGlyph(charCode, fallbackEntry);
	}

	return NULL;
}


inline void
GlyphLayoutEngine::_PopulateAndLockFallbacks(
	BObjectList<FontCacheReference>& fallbacksList,
	const ServerFont& font, bool forceVector)
{
	ASSERT(fallbacksList.IsEmpty());

	// TODO: We always get the fallback glyphs from the Noto family, but of
	// course the fallback font should a) contain the missing glyphs at all
	// and b) be similar to the original font. So there should be a mapping
	// of some kind to know the most suitable fallback font.
	static const char* fallbacks[] = {
		"Noto Sans Display",
		"Noto Sans Thai",
		"Noto Sans CJK JP",
		"Noto Sans Symbols",
		"Noto Sans Symbols2"
	};

	if (!gFontManager->Lock())
		return;

	static const int nStyles = 3;
	static const int nFallbacks = B_COUNT_OF(fallbacks);
	FontCacheEntry* fallbackCacheEntries[nStyles * nFallbacks];
	int entries = 0;

	for (int c = 0; c < nStyles; c++) {
		const char* fontStyle;
		if (c == 0)
			fontStyle = font.Style();
		else if (c == 1)
			fontStyle = "Regular";
		else
			fontStyle = NULL;

		for (int i = 0; i < nFallbacks; i++) {

			FontStyle* fallbackStyle = gFontManager->GetStyle(fallbacks[i],
				fontStyle, 0xffff, 0);

			if (fallbackStyle == NULL)
				continue;

			ServerFont fallbackFont(*fallbackStyle, font.Size());

			FontCacheEntry* entry = FontCacheEntryFor(
				fallbackFont, forceVector);

			if (entry == NULL)
				continue;

			fallbackCacheEntries[entries++] = entry;

		}

	}

	gFontManager->Unlock();

	// Finally lock the entries and save their references
	for (int i = 0; i < entries; i++) {
		FontCacheReference* cacheReference =
			new(std::nothrow) FontCacheReference();
		if (cacheReference != NULL) {
			if (cacheReference->SetTo(fallbackCacheEntries[i], true))
				fallbacksList.AddItem(cacheReference);
			else
				delete cacheReference;
		}
	}

	return;
}


#endif // GLYPH_LAYOUT_ENGINE_H
