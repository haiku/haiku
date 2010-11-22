/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <time.h>
#include <stdio.h>

#include "PosixLCTimeInfo.h"


using BPrivate::Libroot::gPosixLCTimeInfo;


static char*
print_time(char* buffer, size_t bufferSize, const struct tm* tm)
{
	snprintf(buffer, bufferSize, "%.3s %.3s%3d %02d:%02d:%02d %d\n",
		tm->tm_wday < 0 ? "???" : gPosixLCTimeInfo.wday[tm->tm_wday % 7],
		tm->tm_mon < 0 ? "???" : gPosixLCTimeInfo.mon[tm->tm_mon % 12],
		tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
		1900 + tm->tm_year);

	return buffer;
}


extern "C" char*
asctime(const struct tm* tm)
{
	static char buffer[26];
		// That's enough to hold normal dates (i.e. with 4-digit years), for any
		// other dates the behaviour of asctime() is undefined according to the
		// POSIX Base Specifications Issue 7.

	return print_time(buffer, sizeof(buffer), tm);
}


extern "C" char*
asctime_r(const struct tm* tm, char* buffer)
{
	return print_time(buffer, 26, tm);
		// 26 bytes seems to be required by the standard, so we can't write more
}
