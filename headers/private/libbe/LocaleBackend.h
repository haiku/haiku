/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOCALE_BACKEND_H
#define _LOCALE_BACKEND_H


#include <SupportDefs.h>


namespace BPrivate {


class LocaleBackend {
public:
								LocaleBackend();
virtual							~LocaleBackend();

virtual		const char*			GetString(const char* string,
									const char* context = NULL,
									const char* comment = NULL) = 0;

static		status_t			LoadBackend();
};


extern LocaleBackend* gLocaleBackend;


}	// namespace BPrivate


#endif	// _LOCALE_BACKEND_H
