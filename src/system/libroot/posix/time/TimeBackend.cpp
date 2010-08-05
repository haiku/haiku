/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "TimeBackend.h"

#include <dlfcn.h>
#include <pthread.h>


namespace BPrivate {


TimeBackend* gTimeBackend = NULL;


static TimeDataBridge sTimeDataBridge;
static pthread_once_t sBackendInitOnce = PTHREAD_ONCE_INIT;


static void
LoadBackend()
{
	void* imageHandle = dlopen("libroot-addon-icu.so", RTLD_LAZY);
	if (imageHandle == NULL)
		return;

	typedef TimeBackend* (*symbolType)();
	symbolType createInstanceFunc
		= (symbolType)dlsym(imageHandle, "CreateInstance");
	if (createInstanceFunc == NULL)
		return;

	gTimeBackend = createInstanceFunc();
}


TimeBackend::TimeBackend()
{
}


TimeBackend::~TimeBackend()
{
}


status_t
TimeBackend::LoadBackend()
{
	if (gTimeBackend == NULL) {
		pthread_once(&sBackendInitOnce, &BPrivate::LoadBackend);
		if (gTimeBackend != NULL)
			gTimeBackend->Initialize(&sTimeDataBridge);
	}

	return gTimeBackend != NULL ? B_OK : B_ERROR;
}


}	// namespace BPrivate
