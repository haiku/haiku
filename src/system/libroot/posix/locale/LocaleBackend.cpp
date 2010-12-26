/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "LocaleBackend.h"

#include <dlfcn.h>
#include <pthread.h>
#include <string.h>


namespace BPrivate {
namespace Libroot {


LocaleBackend* gLocaleBackend = NULL;


static LocaleDataBridge sLocaleDataBridge;
static pthread_once_t sBackendInitOnce = PTHREAD_ONCE_INIT;


static void
LoadBackend()
{
	void* imageHandle = dlopen("libroot-addon-icu.so", RTLD_LAZY);
	if (imageHandle == NULL)
		return;

	typedef LocaleBackend* (*symbolType)();
	symbolType createInstanceFunc
		= (symbolType)dlsym(imageHandle, "CreateInstance");
	if (createInstanceFunc == NULL) {
		dlclose(imageHandle);
		return;
	}

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
	if (gLocaleBackend == NULL) {
		pthread_once(&sBackendInitOnce, &BPrivate::Libroot::LoadBackend);
		if (gLocaleBackend != NULL)
			gLocaleBackend->Initialize(&sLocaleDataBridge);
	}

	return gLocaleBackend != NULL ? B_OK : B_ERROR;
}


}	// namespace Libroot
}	// namespace BPrivate
