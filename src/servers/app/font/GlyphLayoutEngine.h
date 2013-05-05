/*
 * Copyright 2007, Haiku. All rights reserved.
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

#include <Debug.h>

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

	void SetTo(FontCacheEntry* entry, bool writeLocked)
	{
		// NOTE: If the semantics are changed such
		// that the reference to a previous entry
		// is properly released, then don't forget
		// to adapt existing which transfers
		// responsibility of entries between
		// references!
		fCacheEntry = entry;
		fWriteLocked = writeLocked;
	}

	void Unset()
	{
		if (fCacheEntry == NULL)
			return;

		if (fWriteLocked)
			fCacheEntry->WriteUnlock();
		else
			fCacheEntry->ReadUnlock();

		FontCache::Default()->Recycle(fCacheEntry);
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
									const FontCacheEntry* disallowedEntry,
									const char* utf8String, int32 length,
									FontCacheReference& cacheReference,
									bool needsWriteLock);

			template<class GlyphConsumer>
	static	bool				LayoutGlyphs(GlyphConsumer& consumer,
									const ServerFont& font,
									const char* utf8String,
									int32 length,
									const escapement_delta* delta = NULL,
									bool kerning = true,
									uint8 spacing = B_BITMAP_SPACING,
									const BPoint* offsets = NULL,
									FontCacheReference* cacheReference = NULL);

private:
	static	bool				_WriteLockAndAcquireFallbackEntry(
									FontCacheReference& cacheReference,
									FontCacheEntry* entry,
									const ServerFont& font,
									const char* utf8String, int32 length,
									FontCacheReference& fallbackCacheReference,
									FontCacheEntry*& fallbackEntry);

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
GlyphLayoutEngine::FontCacheEntryFor(const ServerFont& font,
	const FontCacheEntry* disallowedEntry, const char* utf8String, int32 length,
	FontCacheReference& cacheReference, bool needsWriteLock)
{
	ASSERT(cacheReference.Entry() == NULL);

	FontCache* cache = FontCache::Default();
	FontCacheEntry* entry = cache->FontCacheEntryFor(font);
	if (entry == NULL)
		return NULL;

	if (entry == disallowedEntry) {
		cache->Recycle(entry);
		return NULL;
	}

	if (needsWriteLock) {
		if (!entry->WriteLock()) {
			cache->Recycle(entry);
			return NULL;
		}
	} else {
		if (!entry->ReadLock()) {
			cache->Recycle(entry);
			return NULL;
		}
	}

	// At this point, we have a valid FontCacheEntry and it is locked in the
	// proper mode. We can setup the FontCacheReference so it takes care of
	// the locking and recycling from now and return the entry.
	cacheReference.SetTo(entry, needsWriteLock);
	return entry;
}


template<class GlyphConsumer>
inline bool
GlyphLayoutEngine::LayoutGlyphs(GlyphConsumer& consumer,
	const ServerFont& font,
	const char* utf8String, int32 length,
	const escapement_delta* delta, bool kerning, uint8 spacing,
	const BPoint* offsets, FontCacheReference* _cacheReference)
{
	// TODO: implement spacing modes
	FontCacheEntry* entry = NULL;
	FontCacheReference cacheReference;
	FontCacheEntry* fallbackEntry = NULL;
	FontCacheReference fallbackCacheReference;
	if (_cacheReference != NULL) {
		entry = _cacheReference->Entry();
		// When there is already a cacheReference, it means there was already
		// an iteration over the glyphs. The use-case is for example to do
		// a layout pass to get the string width for the bounding box, then a
		// second layout pass to actually render the glyphs to the screen.
		// This means that the fallback entry mechanism will not do any good
		// for the second pass, since the fallback glyphs have been stored in
		// the original entry.
	}

	if (entry == NULL) {
		entry = FontCacheEntryFor(font, NULL, utf8String, length,
			cacheReference, false);

		if (entry == NULL)
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

//	uint32 lastCharCode = 0;
	uint32 charCode;
	int32 index = 0;
	bool writeLocked = false;
	const char* start = utf8String;
	while ((charCode = UTF8ToCharCode(&utf8String))) {

		if (offsets) {
			// Use direct glyph locations instead of calculating them
			// from the advance values
			x = offsets[index].x;
			y = offsets[index].y;
		} else {
// TODO: Currently disabled, because it works much too slow (doesn't seem
// to be properly cached in FreeType.)
//			if (kerning)
//				entry->GetKerning(lastCharCode, charCode, &advanceX, &advanceY);

			x += advanceX;
			y += advanceY;

			if (delta)
				x += IsWhiteSpace(charCode) ? delta->space : delta->nonspace;
		}

		const GlyphCache* glyph = entry->CachedGlyph(charCode);
		if (glyph == NULL) {
			// The glyph has not been cached yet, switch to a write lock,
			// acquire the fallback entry and create the glyph. Note that
			// the write lock will persist (in the cacheReference) so that
			// we only have to do this switch once for the whole string.
			if (!writeLocked) {
				writeLocked = _WriteLockAndAcquireFallbackEntry(cacheReference,
					entry, font, utf8String, length, fallbackCacheReference,
					fallbackEntry);
			}

			if (writeLocked)
				glyph = entry->CreateGlyph(charCode, fallbackEntry);
		}

		if (glyph == NULL) {
			consumer.ConsumeEmptyGlyph(index++, charCode, x, y);
			advanceX = 0;
			advanceY = 0;
		} else {
			if (!consumer.ConsumeGlyph(index++, charCode, glyph, entry, x, y)) {
				advanceX = 0;
				advanceY = 0;
				break;
			}

			// get next increment for pen position
			advanceX = glyph->advance_x;
			advanceY = glyph->advance_y;
		}

//		lastCharCode = charCode;
		if (utf8String - start + 1 > length)
			break;
	}

	x += advanceX;
	y += advanceY;
	consumer.Finish(x, y);

	if (_cacheReference != NULL && _cacheReference->Entry() == NULL) {
		// The caller passed a FontCacheReference, but this is the first
		// iteration -> switch the ownership from the stack allocated
		// FontCacheReference to the one passed by the caller. The fallback
		// FontCacheReference is not affected by this, since it is never used
		// during a second iteration.
		_cacheReference->SetTo(entry, cacheReference.WriteLocked());
		cacheReference.SetTo(NULL, false);
	}
	return true;
}


inline bool
GlyphLayoutEngine::_WriteLockAndAcquireFallbackEntry(
	FontCacheReference& cacheReference, FontCacheEntry* entry,
	const ServerFont& font, const char* utf8String, int32 length,
	FontCacheReference& fallbackCacheReference, FontCacheEntry*& fallbackEntry)
{
	// We need the fallback font, since potentially, we have to obtain missing
	// glyphs from it. We need to obtain the fallback font while we have not
	// locked anything, since locking the FontManager with the write-lock held
	// can obvisouly lead to a deadlock.

	cacheReference.SetTo(NULL, false);
	entry->ReadUnlock();

	if (gFontManager->Lock()) {
		// TODO: We always get the fallback glyphs from VL Gothic at the
		// moment, but of course the fallback font should a) contain the
		// missing glyphs at all and b) be similar to the original font.
		// So there should be a mapping of some kind to know the most
		// suitable fallback font.
		FontStyle* fallbackStyle = gFontManager->GetStyleByIndex(
			"VL Gothic", 0);
		if (fallbackStyle != NULL) {
			ServerFont fallbackFont(*fallbackStyle, font.Size());
			gFontManager->Unlock();
			// Force the write-lock on the fallback entry, since we
			// don't transfer or copy GlyphCache objects from one cache
			// to the other, but create new glyphs which are stored in
			// "entry" in any case, which requires the write cache for
			// sure (used FontEngine of fallbackEntry).
			fallbackEntry = FontCacheEntryFor(fallbackFont, entry,
				utf8String, length, fallbackCacheReference, true);
			// NOTE: We don't care if fallbackEntry is NULL, fetching
			// alternate glyphs will simply not work.
		} else
			gFontManager->Unlock();
	}

	if (!entry->WriteLock()) {
		FontCache::Default()->Recycle(entry);
		return false;
	}

	// Update the FontCacheReference, since the locking kind changed.
	cacheReference.SetTo(entry, true);
	return true;
}


#endif // GLYPH_LAYOUT_ENGINE_H
