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
#include "ServerFont.h"

#include <ctype.h>

class FontCacheReference {
 public:
								FontCacheReference()
									: fCacheEntry(NULL)
									, fWriteLocked(false)
								{
								}
								~FontCacheReference()
								{
									if (fCacheEntry) {
										if (fWriteLocked)
											fCacheEntry->WriteUnlock();
										else
											fCacheEntry->ReadUnlock();
									
										FontCache::Default()->Recycle(
											fCacheEntry);
									}
								}
			void				SetTo(FontCacheEntry* entry,
									bool writeLocked)
								{
									fCacheEntry = entry;
									fWriteLocked = writeLocked;
								}
	inline	FontCacheEntry*		Entry() const
									{ return fCacheEntry; }
	inline	bool				WriteLocked() const
									{ return fWriteLocked; }

 private:
			FontCacheEntry*		fCacheEntry;
			bool				fWriteLocked;
};


class GlyphLayoutEngine {
 public:

			template<class GlyphConsumer>
	static	bool				LayoutGlyphs(GlyphConsumer& consumer,
									const ServerFont& font,
									const char* utf8String,
									int32 length,
									const escapement_delta* delta = NULL,
									bool kerning = true,
									uint8 spacing = B_BITMAP_SPACING,
									FontCacheReference* cacheReference = NULL);

	static	bool				IsWhiteSpace(uint32 glyphCode);

 private:
								GlyphLayoutEngine();
	virtual						~GlyphLayoutEngine();
};


// IsWhiteSpace
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

// LayoutGlyphs
template<class GlyphConsumer>
inline bool
GlyphLayoutEngine::LayoutGlyphs(GlyphConsumer& consumer,
	const ServerFont& font,
	const char* utf8String, int32 length,
	const escapement_delta* delta, bool kerning, uint8 spacing,
	FontCacheReference* cacheReference)
{
	// TODO: implement spacing modes

	FontCacheEntry* entry = NULL;
	bool needsWriteLock = false;
	if (cacheReference) {
		entry = cacheReference->Entry();
		needsWriteLock = cacheReference->WriteLocked();
	}

	if (!entry) {
		FontCache* cache = FontCache::Default();
		entry = cache->FontCacheEntryFor(font);
	
		if (!entry || !entry->ReadLock()) {
			cache->Recycle(entry);
			return false;
		}
	
		needsWriteLock = !entry->HasGlyphs(utf8String, length);
	
		if (needsWriteLock) {
			entry->ReadUnlock();
			if (!entry->WriteLock()) {
				cache->Recycle(entry);
				return false;
			}
		}
	} // else the entry was already used and is still locked

	consumer.Start();

	double x = 0.0;
	double y = 0.0;

	double advanceX = 0.0;
	double advanceY = 0.0;

	uint32 lastCharCode = 0;
	uint32 charCode;
	int32 index = 0;
	const char* start = utf8String;
	while ((charCode = UTF8ToCharCode(&utf8String))) {

		const GlyphCache* glyph = entry->Glyph(charCode);
		if (glyph == NULL) {
			fprintf(stderr, "failed to load glyph for 0x%04lx (%c)\n", charCode,
				isprint(charCode) ? (char)charCode : '-');

			consumer.ConsumeEmptyGlyph(index, charCode, x, y);
			continue;
		}

//		if (kerning)
//			entry->GetKerning(lastCharCode, charCode, &advanceX, &advanceY);

		x += advanceX;
		y += advanceY;

		if (delta)
			x += IsWhiteSpace(charCode) ? delta->space : delta->nonspace;

		if (!consumer.ConsumeGlyph(index, charCode, glyph, entry, x, y))
			break;

		// increment pen position
		advanceX = glyph->advance_x;
		advanceY = glyph->advance_y;

		lastCharCode = charCode;
		index++;
		if (utf8String - start + 1 > length)
			break;
	}

	x += advanceX;
	y += advanceY;
	consumer.Finish(x, y);

	if (!cacheReference) {
		if (needsWriteLock)
			entry->WriteUnlock();
		else
			entry->ReadUnlock();
	
		FontCache::Default()->Recycle(entry);
	} else {
		// the reference will take care of locking
		if (!cacheReference->Entry())
			cacheReference->SetTo(entry, needsWriteLock);
	}
	return true;
}


#endif // GLYPH_LAYOUT_ENGINE_H
