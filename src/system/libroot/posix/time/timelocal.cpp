/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "timelocal.h"

#include <stddef.h>

#include "LocaleBackend.h"
#include "PosixLCTimeInfo.h"


using BPrivate::Libroot::GetCurrentLocaleBackend;
using BPrivate::Libroot::LocaleBackend;
using BPrivate::Libroot::LocaleBackendData;


extern "C" const lc_time_t*
__get_current_time_locale(void)
{
	if (GetCurrentLocaleBackend() != NULL)
		return GetCurrentLocaleBackend()->LCTimeInfo();

	return &BPrivate::Libroot::gPosixLCTimeInfo;
}


extern "C" const lc_time_t*
__get_time_locale(locale_t l)
{
	LocaleBackendData* locale = (LocaleBackendData*)l;
	LocaleBackend* backend = locale->backend;

	if (backend != NULL)
		return backend->LCTimeInfo();

	return &BPrivate::Libroot::gPosixLCTimeInfo;
}
