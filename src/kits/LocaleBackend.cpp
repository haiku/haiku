/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "LocaleBackend.h"

#include <dlfcn.h>
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
	void* imageHandle = dlopen("liblocale.so", RTLD_LAZY);
	if (imageHandle == NULL)
		return;

	typedef LocaleBackend* (*symbolType)();
	symbolType createInstanceFunc = (symbolType)dlsym(imageHandle,
		"CreateLocaleBackendInstance");
	if (createInstanceFunc == NULL)
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
