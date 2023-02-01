/*
 * Copyright 2007-2022, Haiku. All rights reserved.
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
#include "GlobalFontManager.h"
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
		fFallbackReference(NULL),
		fLocked(false),
		fWriteLocked(false)
	{
	}

	~FontCacheReference()
	{
		if (fCacheEntry == NULL)
			return;

		fFallbackReference = NULL;
		Unlock();
		if (fCacheEntry != NULL)
			FontCache::Default()->Recycle(fCacheEntry);
	}

	void SetTo(FontCacheEntry* entry)
	{
		ASSERT(entry != NULL);
		ASSERT(fCacheEntry == NULL);

		fCacheEntry = entry;
	}

	bool ReadLock()
	{
		ASSERT(fCacheEntry != NULL);
		ASSERT(fWriteLocked == false);

		if (fLocked)
			return true;

		if (!fCacheEntry->ReadLock()) {
			_Cleanup();
			return false;
		}

		fLocked = true;
		return true;
	}

	bool WriteLock()
	{
		ASSERT(fCacheEntry != NULL);

		if (fWriteLocked)
			return true;

		if (fLocked) {
			if (!fCacheEntry->ReadUnlock()) {
				_Cleanup();
				return false;
			}
		}
		if (!fCacheEntry->WriteLock()) {
			_Cleanup();
			return false;
		}

		fLocked = true;
		fWriteLocked = true;
		return true;
	}

	bool Unlock()
	{
		ASSERT(fCacheEntry != NULL);

		if (!fLocked)
			return true;

		if (fWriteLocked) {
			if (!fCacheEntry->WriteUnlock()) {
				_Cleanup();
				return false;
			}
		} else {
			if (!fCacheEntry->ReadUnlock()) {
				_Cleanup();
				return false;
			}
		}

		fLocked = false;
		fWriteLocked = false;
		return true;
	}

	bool SetFallback(FontCacheReference* fallback)
	{
		ASSERT(fCacheEntry != NULL);
		ASSERT(fallback != NULL);
		ASSERT(fallback->Entry() != NULL);
		ASSERT(fallback->Entry() != fCacheEntry);

		if (fFallbackReference == fallback)
			return true;

		if (fFallbackReference != NULL) {
			fFallbackReference->Unlock();
			fFallbackReference = NULL;
		}

		// We need to create new glyphs with the engine of the fallback font
		// and store them in the main font cache (not just transfer them from
		// one cache to the other). So we need both to be write-locked.
		if (fallback->Entry() < fCacheEntry) {
			if (fLocked && !Unlock())
				return false;
			if (!fallback->WriteLock()) {
				WriteLock();
				return false;
			}
			fFallbackReference = fallback;
			return WriteLock();
		}
		if (fLocked && !fWriteLocked && !Unlock())
			return false;
		if (!WriteLock() || !fallback->WriteLock())
			return false;
		fFallbackReference = fallback;
		return true;
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

	void _Cleanup()
	{
		if (fFallbackReference != NULL) {
			fFallbackReference->Unlock();
			fFallbackReference = NULL;
		}
		if (fCacheEntry != NULL)
			FontCache::Default()->Recycle(fCacheEntry);
		fCacheEntry = NULL;
		fLocked = false;
		fWriteLocked = false;
	}

private:
			FontCacheEntry*		fCacheEntry;
			FontCacheReference*	fFallbackReference;
			bool				fLocked;
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

	static	void				PopulateFallbacks(
									BObjectList<FontCacheReference>& fallbacks,
									const ServerFont& font, bool forceVector);

	static FontCacheReference*	GetFallbackReference(
									BObjectList<FontCacheReference>& fallbacks,
									uint32 charCode);

private:
	static	const GlyphCache*	_CreateGlyph(
									FontCacheReference& cacheReference,
									BObjectList<FontCacheReference>& fallbacks,
									const ServerFont& font, bool needsVector,
									uint32 glyphCode);

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
		pCacheReference->SetTo(entry);
		if (!pCacheReference->ReadLock())
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

	// Avoid loading the fallbacks if our font can create the glyph.
	if (entry->CanCreateGlyph(charCode)) {
		if (cacheReference.WriteLock())
			return entry->CreateGlyph(charCode);
		return NULL;
	}

	if (fallbacks.IsEmpty())
		PopulateFallbacks(fallbacks, font, forceVector);

	FontCacheReference* fallbackReference = GetFallbackReference(fallbacks, charCode);
	if (fallbackReference != NULL) {
		if (cacheReference.SetFallback(fallbackReference))
			return entry->CreateGlyph(charCode, fallbackReference->Entry());
		if (cacheReference.Entry() == NULL)
			return NULL;
	}

	// No one knows how to draw this, so use the missing glyph symbol.
	if (cacheReference.WriteLock())
		return entry->CreateGlyph(charCode);
	return NULL;
}


inline void
GlyphLayoutEngine::PopulateFallbacks(
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
		"Noto Sans Symbols2",
		"Noto Emoji",
	};

	if (!gFontManager->Lock())
		return;

	static const int nFallbacks = B_COUNT_OF(fallbacks);

	for (int c = 0; c < 3; c++) {
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

			FontCacheEntry* entry = FontCacheEntryFor(fallbackFont, forceVector);
			if (entry == NULL)
				continue;

			FontCacheReference* cacheReference = new(std::nothrow) FontCacheReference();
			if (cacheReference != NULL) {
				cacheReference->SetTo(entry);
				fallbacksList.AddItem(cacheReference);
			} else
				FontCache::Default()->Recycle(entry);

		}

	}

	gFontManager->Unlock();
}


inline FontCacheReference*
GlyphLayoutEngine::GetFallbackReference(
	BObjectList<FontCacheReference>& fallbacks, uint32 charCode)
{
	int32 count = fallbacks.CountItems();
	for (int32 index = 0; index < count; index++) {
		FontCacheReference* fallbackReference = fallbacks.ItemAt(index);
		FontCacheEntry* fallbackEntry = fallbackReference->Entry();
		if (fallbackEntry != NULL && fallbackEntry->CanCreateGlyph(charCode))
			return fallbackReference;
	}
	return NULL;
}


#endif // GLYPH_LAYOUT_ENGINE_H
