/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "LocaleBackend.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include <image.h>


namespace BPrivate {


LocaleBackend* gLocaleBackend = NULL;


static pthread_once_t sBackendInitOnce = PTHREAD_ONCE_INIT;


static void
LoadBackend()
{
	image_id libLocaleAddonID = load_add_on("/system/lib/liblocale.so");

	if (libLocaleAddonID < 0)
		return;
	LocaleBackend* (*createInstanceFunc)();
	if (get_image_symbol(libLocaleAddonID, "CreateLocaleBackendInstance", 
			B_SYMBOL_TYPE_TEXT, (void**)&createInstanceFunc) != B_OK)
		return;

	gLocaleBackend = createInstanceFunc();
}


LocaleBackend::LocaleBackend()
{
}


LocaleBackend::~LocaleBackend()
{
}


status_t
LocaleBackend::LoadBackend()
{
	if (gLocaleBackend == NULL)
		pthread_once(&sBackendInitOnce, &BPrivate::LoadBackend);

	return gLocaleBackend != NULL ? B_OK : B_ERROR;
}


}	// namespace BPrivate
