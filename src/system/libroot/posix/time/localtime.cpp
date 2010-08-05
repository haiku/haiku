/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>

#include <localtime.h>

#include <FindDirectory.h>
#include <StorageDefs.h>


char* tzname[2] = { "???", "???" };
long timezone = 0;
int daylight = 0;


namespace BPrivate {


static time_zone_info tzInfo;


extern "C" void
tzset(void)
{
	char path[B_PATH_NAME_LENGTH];
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, false, path,
			sizeof(path)) < 0)
		return;
	strlcat(path, "/", sizeof(path));
	strlcat(path, skPosixTimeZoneInfoFile, sizeof(path));

	FILE* tzInfoFile = fopen(path, "r");
	if (tzInfoFile == NULL)
		return;

	size_t numRead = fread(&tzInfo, sizeof(tzInfo), 1, tzInfoFile);
	fclose(tzInfoFile);

	if (numRead != 1)
		return;

	tzname[0] = tzInfo.short_std_name;
	tzname[1] = tzInfo.short_dst_name;
	timezone = tzInfo.offset_from_gmt;
	daylight = tzInfo.uses_daylight_saving;
}


}	// namespace BPrivate
