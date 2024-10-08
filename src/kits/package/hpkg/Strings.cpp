/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/Strings.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


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


void
StringCache::Put(CachedString* string)
{
	if (string != NULL) {
		if (--string->usageCount == 0) {
			Remove(string);
			delete string;
		}
	}
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
