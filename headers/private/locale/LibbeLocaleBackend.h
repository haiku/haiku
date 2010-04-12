/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBBE_LOCALE_BACKEND_H
#define _LIBBE_LOCALE_BACKEND_H


#include <LocaleBackend.h>

class BCatalogAddOn;

namespace BPrivate {


class LibbeLocaleBackend : public LocaleBackend {
public:
								LibbeLocaleBackend();
virtual							~LibbeLocaleBackend();

virtual		const char*			GetString(const char* string,
									const char* context = NULL,
									const char* comment = NULL);
private:
	BCatalogAddOn* systemCatalog;
};


}	// namespace BPrivate


#endif	// _LIBBE_LOCALE_BACKEND_H
