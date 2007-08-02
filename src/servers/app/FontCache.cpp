/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "FontCache.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Entry.h>
#include <Path.h>

#include "AutoLocker.h"


using std::nothrow;


FontCache
FontCache::sDefaultInstance;

// #pragma mark -

// constructor
FontCache::FontCache()
	: MultiLocker("FontCache lock")
	, fFontCacheEntries()
{
}

// destructor
FontCache::~FontCache()
{
	FontMap::Iterator iterator = fFontCacheEntries.GetIterator();
	while (iterator.HasNext())
		iterator.Next().value->RemoveReference();
}

// Default
/*static*/ FontCache*
FontCache::Default()
{
	return &sDefaultInstance;
}

// SetFont
FontCacheEntry*
FontCache::FontCacheEntryFor(const ServerFont& font)
{
	char signature[512];
	FontCacheEntry::GenerateSignature(signature, font);

	AutoReadLocker readLocker(this);

	FontCacheEntry* entry = fFontCacheEntries.Get(signature);

	if (entry) {
		// the entry was already there
		entry->AddReference();
		return entry;
	}

	readLocker.Unlock();

	AutoWriteLocker locker(this);
	if (!locker.IsLocked())
		return NULL;

	// prevent getting screwed by a race condition:
	// when we released the readlock above, another thread might have
	// gotten the writelock before we have, and might have already
	// inserted a cache entry for this font. So we look again if there
	// is an entry now, and only then create it if it's still not there,
	// all while holding the writelock
	entry = fFontCacheEntries.Get(signature);

	if (!entry) {
		// remove old entries, keep entries below certain count
		_ConstrainEntryCount();
		entry = new (nothrow) FontCacheEntry();
		if (!entry || !entry->Init(font)
			|| fFontCacheEntries.Put(signature, entry) < B_OK) {
			fprintf(stderr, "FontCache::FontCacheEntryFor() - "
				"out of memory or no font file\n");
			delete entry;
			return NULL;
		}
	}

	entry->AddReference();
	return entry;
}

// Recycle
void
FontCache::Recycle(FontCacheEntry* entry)
{
	entry->UpdateUsage();
	entry->RemoveReference();
}

static const int32 kMaxEntryCount = 30;

static inline double
usage_index(uint64 useCount, bigtime_t age)
{
	return 100.0 * useCount / age;
}

// _ConstrainEntryCount
void
FontCache::_ConstrainEntryCount()
{
	// this function is only ever called with the WriteLock held
	if (fFontCacheEntries.Size() < kMaxEntryCount)
		return;
//printf("FontCache::_ConstrainEntryCount()\n");

	FontMap::Iterator iterator = fFontCacheEntries.GetIterator();

	// NOTE: if kMaxEntryCount has a sane value, there has got to be
	// some entries, so using the iterator like that should be ok
	FontCacheEntry* leastUsedEntry = iterator.Next().value;
	bigtime_t now = system_time();
	bigtime_t age = now - leastUsedEntry->LastUsed();
	uint64 useCount = leastUsedEntry->UsedCount();
	double leastUsageIndex = usage_index(useCount, age);
//printf("  leastUsageIndex: %f\n", leastUsageIndex);

	while (iterator.HasNext()) {
		FontCacheEntry* entry = iterator.Next().value;
		age = now - entry->LastUsed();
		useCount = entry->UsedCount();
		double usageIndex = usage_index(useCount, age);
//printf("  usageIndex: %f\n", usageIndex);
		if (usageIndex < leastUsageIndex) {
			leastUsedEntry = entry;
			leastUsageIndex = usageIndex;
		}
	}

	iterator = fFontCacheEntries.GetIterator();
	while (iterator.HasNext()) {
		if (iterator.Next().value == leastUsedEntry) {
			iterator.Remove();
			leastUsedEntry->RemoveReference();
			break;
		}
	}
}
