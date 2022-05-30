/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "LocaleBackend.h"

#include <dlfcn.h>
#include <pthread.h>
#include <string.h>

#include <ThreadLocale.h>


namespace BPrivate {
namespace Libroot {


LocaleBackend* gGlobalLocaleBackend = NULL;
LocaleDataBridge gGlobalLocaleDataBridge = LocaleDataBridge(true);


static void* sImageHandle = NULL;
typedef LocaleBackend* (*create_instance_t)();
typedef void (*destroy_instance_t)(LocaleBackend*);
static create_instance_t sCreateInstanceFunc = NULL;
static destroy_instance_t sDestroyInstanceFunc = NULL;


static pthread_once_t sBackendInitOnce = PTHREAD_ONCE_INIT;
static pthread_once_t sFunctionsInitOnce = PTHREAD_ONCE_INIT;


static void
LoadFunctions()
{
	sImageHandle = dlopen("libroot-addon-icu.so", RTLD_LAZY);
	if (sImageHandle == NULL)
		return;

	sCreateInstanceFunc = (create_instance_t)dlsym(sImageHandle, "CreateInstance");
	sDestroyInstanceFunc = (destroy_instance_t)dlsym(sImageHandle, "DestroyInstance");

	if ((sCreateInstanceFunc == NULL) || (sDestroyInstanceFunc == NULL)) {
		dlclose(sImageHandle);
		return;
	}
}


static void
LoadBackend()
{
	gGlobalLocaleBackend = LocaleBackend::CreateBackend();
	if (gGlobalLocaleBackend != NULL) {
		gGlobalLocaleBackend->Initialize(&gGlobalLocaleDataBridge);
	}
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
	if (gGlobalLocaleBackend == NULL) {
		pthread_once(&sBackendInitOnce, &BPrivate::Libroot::LoadBackend);
	}

	return gGlobalLocaleBackend != NULL ? B_OK : B_ERROR;
}


LocaleBackend*
LocaleBackend::CreateBackend()
{
	if (sCreateInstanceFunc == NULL) {
		pthread_once(&sFunctionsInitOnce, &BPrivate::Libroot::LoadFunctions);
	}

	if (sCreateInstanceFunc != NULL) {
		LocaleBackend* backend = sCreateInstanceFunc();
		return backend;
	}

	return NULL;
}


void
LocaleBackend::DestroyBackend(LocaleBackend* instance)
{
	if (sDestroyInstanceFunc != NULL) {
		sDestroyInstanceFunc(instance);
	}
}


LocaleBackendData* GetCurrentLocaleInfo()
{
	return GetCurrentThreadLocale()->threadLocaleInfo;
}


void
SetCurrentLocaleInfo(LocaleBackendData* newLocale)
{
	GetCurrentThreadLocale()->threadLocaleInfo = newLocale;
}


LocaleBackend* GetCurrentLocaleBackend()
{
	LocaleBackendData* info = GetCurrentThreadLocale()->threadLocaleInfo;
	if (info != NULL && info->backend != NULL) {
		return info->backend;
	}
	return gGlobalLocaleBackend;
}

}	// namespace Libroot
}	// namespace BPrivate
