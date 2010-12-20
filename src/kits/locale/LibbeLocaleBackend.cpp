/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "LibbeLocaleBackend.h"

#include "Catalog.h"
#include "Locale.h"
#include "MutableLocaleRoster.h"

#include <new>


namespace BPrivate {


extern "C" LocaleBackend*
CreateLocaleBackendInstance()
{
	return new(std::nothrow) LibbeLocaleBackend();
}


LibbeLocaleBackend::LibbeLocaleBackend()
{
	MutableLocaleRoster::Default()->GetSystemCatalog(&systemCatalog);
}


LibbeLocaleBackend::~LibbeLocaleBackend()
{
}


const char*
LibbeLocaleBackend::GetString(const char* string, const char* context,
	const char* comment)
{
	// The system catalog will not be there for non-localized apps.
	if (systemCatalog) {
		const char *translated = systemCatalog->GetString(string, context, comment);
		if (translated)
			return translated;
	}
	return string;
}


}	// namespace BPrivate
