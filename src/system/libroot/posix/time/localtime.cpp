/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <FindDirectory.h>
#include <StorageDefs.h>

#include "LocaleBackend.h"


using BPrivate::gLocaleBackend;
using BPrivate::LocaleBackend;


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

	char timeZoneID[64] = { "GMT" };

	const char* tz = getenv("TZ");
	if (tz != NULL)
		strlcpy(timeZoneID, tz, sizeof(timeZoneID));
	else {
		do {
			char path[B_PATH_NAME_LENGTH];
			if (find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, false, path,
					sizeof(path)) < 0)
				break;
			strlcat(path, "/libroot_timezone_info", sizeof(path));

			FILE* tzInfoFile = fopen(path, "r");
			if (tzInfoFile == NULL)
				break;

			fgets(timeZoneID, sizeof(timeZoneID), tzInfoFile);
			fclose(tzInfoFile);
		} while(0);
	}

	if (gLocaleBackend != NULL) {
		gLocaleBackend->TZSet(timeZoneID);
	}
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
		errno = EINVAL;
		return NULL;
	}

	tzset();
	if (gLocaleBackend != NULL) {
		status_t status = gLocaleBackend->Localtime(inTime, tmOut);

		if (status != B_OK)
			errno = EOVERFLOW;

		return tmOut;
	}

	errno = ENOSYS;
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
		errno = EINVAL;
		return NULL;
	}

	tzset();
	if (gLocaleBackend != NULL) {
		status_t status = gLocaleBackend->Gmtime(inTime, tmOut);

		if (status != B_OK)
			errno = EOVERFLOW;

		return tmOut;
	}

	errno = ENOSYS;
	return NULL;
}


extern "C" time_t
mktime(struct tm* inTm)
{
	if (inTm == NULL) {
		errno = EINVAL;
		return -1;
	}

	tzset();
	if (gLocaleBackend != NULL) {
		time_t timeOut;
		status_t status = gLocaleBackend->Mktime(inTm, timeOut);

		if (status != B_OK) {
			errno = EOVERFLOW;
			return -1;
		}

		return timeOut;
	}

	errno = ENOSYS;
	return -1;
}
