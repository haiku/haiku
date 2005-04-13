/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <time.h>
#include <stdio.h>


static char *
print_time(char *buffer, size_t bufferSize, const struct tm *tm)
{
	// ToDo: this should probably use the locale kit to get these names

	static const char weekdays[][3] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char months[][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	snprintf(buffer, bufferSize, "%.3s %.3s%3d %02d:%02d:%02d %d\n",
		tm->tm_wday < 0 ? "???" : weekdays[tm->tm_wday % 7],
		tm->tm_mon < 0 ? "???" : months[tm->tm_mon % 12],
		tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
		1900 + tm->tm_year);

	return buffer;
}


char *
asctime(const struct tm *tm)
{
	static char buffer[28];
		// is enough to hold normal dates

	return print_time(buffer, sizeof(buffer), tm);
}


char *
asctime_r(const struct tm *tm, char *buffer)
{
	return print_time(buffer, 26, tm);
		// 26 bytes seems to be required by the standard, so we can't write more
}

