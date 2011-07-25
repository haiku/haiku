/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef FONT_CACHE_H
#define FONT_CACHE_H

#include "FontCacheEntry.h"
#include "HashMap.h"
#include "HashString.h"
#include "MultiLocker.h"
#include "ServerFont.h"


class FontCache : public MultiLocker {
 public:
								FontCache();
	virtual						~FontCache();

	// global instance
	static	FontCache*			Default();

			FontCacheEntry*		FontCacheEntryFor(const ServerFont& font);
			void				Recycle(FontCacheEntry* entry);

 private:
			void				_ConstrainEntryCount();

	static	FontCache			sDefaultInstance;

	typedef HashMap<HashString, FontCacheEntry*> FontMap;

			FontMap				fFontCacheEntries;
};

#endif // FONT_CACHE_H
