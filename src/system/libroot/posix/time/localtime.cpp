/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <syscalls.h>

#include <StorageDefs.h>

#include <errno_private.h>
#include "LocaleBackend.h"


using BPrivate::Libroot::GetCurrentLocaleBackend;
using BPrivate::Libroot::LocaleBackend;


static char sStandardTZName[64] 		= { "GMT" };
static char sDaylightSavingTZName[64]	= { "GMT" };


char* tzname[2] = {
	sStandardTZName,
	sDaylightSavingTZName
};
long timezone = 0;
int daylight = 0;


// These three functions are used as a fallback when the locale backend could not
// be loaded, or for the POSIX locale. They are implemented in localtime_fallback.c.
extern "C" struct tm* __gmtime_r_fallback(const time_t* timep, struct tm* tmp);
extern "C" time_t __mktime_fallback(struct tm* tmp);
extern "C" time_t __timegm_fallback(struct tm* tmp);


extern "C" void
tzset(void)
{
	if (GetCurrentLocaleBackend() == NULL && LocaleBackend::LoadBackend() != B_OK)
		return;

	char timeZoneID[B_FILE_NAME_LENGTH] = { "GMT" };
	_kern_get_timezone(NULL, timeZoneID, sizeof(timeZoneID));

	GetCurrentLocaleBackend()->TZSet(timeZoneID, getenv("TZ"));
}


extern "C" struct tm*
localtime(const time_t* inTime)
{
	static tm tm;

	return localtime_r(inTime, &tm);
}


extern "C" struct tm*
localtime_r(const time_t* inTime, struct tm* tmOut)
{
	if (inTime == NULL) {
		__set_errno(EINVAL);
		return NULL;
	}

	tzset();

	LocaleBackend* backend = GetCurrentLocaleBackend();

	if (backend != NULL) {
		status_t status = backend->Localtime(inTime, tmOut);

		if (status != B_OK)
			__set_errno(EOVERFLOW);

		return tmOut;
	}

	// without a locale backend, there are no timezones, so we fall back to
	// using a basic gmtime_r implementation.
	return __gmtime_r_fallback(inTime, tmOut);
}


extern "C" struct tm*
gmtime(const time_t* inTime)
{
	static tm tm;

	return gmtime_r(inTime, &tm);
}


extern "C" struct tm*
gmtime_r(const time_t* inTime, struct tm* tmOut)
{
	if (inTime == NULL) {
		__set_errno(EINVAL);
		return NULL;
	}

	tzset();

	LocaleBackend* backend = GetCurrentLocaleBackend();

	if (backend != NULL) {
		status_t status = backend->Gmtime(inTime, tmOut);

		if (status != B_OK)
			__set_errno(EOVERFLOW);

		return tmOut;
	}

	// without a locale backend, we fall back to using a basic gmtime_r
	// implementation.
	return __gmtime_r_fallback(inTime, tmOut);
}


extern "C" time_t
mktime(struct tm* inTm)
{
	if (inTm == NULL) {
		__set_errno(EINVAL);
		return -1;
	}

	tzset();

	LocaleBackend* backend = GetCurrentLocaleBackend();

	if (backend != NULL) {
		time_t timeOut;
		status_t status = backend->Mktime(inTm, timeOut);

		if (status != B_OK) {
			__set_errno(EOVERFLOW);
			return -1;
		}

		return timeOut;
	}

	// without a locale backend, we fall back to using a basic gmtime_r
	// implementation.
	return __mktime_fallback(inTm);
}


extern "C" time_t
timegm(struct tm* inTm)
{
	if (inTm == NULL) {
		__set_errno(EINVAL);
		return -1;
	}
	tzset();

	LocaleBackend* backend = GetCurrentLocaleBackend();

	if (backend != NULL) {
		time_t timeOut;
		status_t status = backend->Timegm(inTm, timeOut);

		if (status != B_OK) {
			__set_errno(EOVERFLOW);
			return -1;
		}

		return timeOut;
	}

	// without a locale backend, we fall back to using a basic timegm
	// implementation.
	return __timegm_fallback(inTm);
}
