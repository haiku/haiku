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


using BPrivate::Libroot::gLocaleBackend;
using BPrivate::Libroot::LocaleBackend;


static char sStandardTZName[64] 		= { "???" };
static char sDaylightSavingTZName[64]	= { "???" };


char* tzname[2] = {
	sStandardTZName,
	sDaylightSavingTZName
};
long timezone = 0;
int daylight = 0;


extern "C" void
tzset(void)
{
	if (gLocaleBackend == NULL && LocaleBackend::LoadBackend() != B_OK)
		return;

	char timeZoneID[B_FILE_NAME_LENGTH] = { "GMT" };
	_kern_get_timezone(NULL, timeZoneID, sizeof(timeZoneID));

	gLocaleBackend->TZSet(timeZoneID, getenv("TZ"));
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
	if (gLocaleBackend != NULL) {
		status_t status = gLocaleBackend->Localtime(inTime, tmOut);

		if (status != B_OK)
			__set_errno(EOVERFLOW);

		return tmOut;
	}

	__set_errno(ENOSYS);
	return NULL;
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
	if (gLocaleBackend != NULL) {
		status_t status = gLocaleBackend->Gmtime(inTime, tmOut);

		if (status != B_OK)
			__set_errno(EOVERFLOW);

		return tmOut;
	}

	__set_errno(ENOSYS);
	return NULL;
}


extern "C" time_t
mktime(struct tm* inTm)
{
	if (inTm == NULL) {
		__set_errno(EINVAL);
		return -1;
	}

	tzset();
	if (gLocaleBackend != NULL) {
		time_t timeOut;
		status_t status = gLocaleBackend->Mktime(inTm, timeOut);

		if (status != B_OK) {
			__set_errno(EOVERFLOW);
			return -1;
		}

		return timeOut;
	}

	__set_errno(ENOSYS);
	return -1;
}
