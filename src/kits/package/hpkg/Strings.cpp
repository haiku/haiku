/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/Strings.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


// from the Dragon Book: a slightly modified hashpjw()
uint32
hash_string(const char* string)
{
	if (string == NULL)
		return 0;

	uint32 h = 0;

	for (; *string; string++) {
		uint32 g = h & 0xf0000000;
		if (g)
			h ^= g >> 24;
		h = (h << 4) + *string;
	}

	return h;
}


StringCache::StringCache()
{
}


StringCache::~StringCache()
{
	CachedString* cachedString = Clear(true);
	while (cachedString != NULL) {
		CachedString* next = cachedString->next;
		delete cachedString;
		cachedString = next;
	}
}


CachedString*
StringCache::Get(const char* value)
{
	CachedString* string = Lookup(value);
	if (string != NULL) {
		string->usageCount++;
		return string;
	}

	string = new CachedString;
	if (!string->Init(value)) {
		delete string;
		throw std::bad_alloc();
	}

	Insert(string);
	return string;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
