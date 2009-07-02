/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <FindDirectory.h>
#include <OS.h>

#include <commpage_defs.h>
#include <libroot_private.h>
#include <real_time_data.h>
#include <syscalls.h>


static struct real_time_data* sRealTimeData;


void
__init_time(void)
{
	sRealTimeData = (struct real_time_data*)
		USER_COMMPAGE_TABLE[COMMPAGE_ENTRY_REAL_TIME_DATA];

	__arch_init_time(sRealTimeData, false);
}


//	#pragma mark - public API


uint32
real_time_clock(void)
{
	return (__arch_get_system_time_offset(sRealTimeData) + system_time())
		/ 1000000;
}


bigtime_t
real_time_clock_usecs(void)
{
	return __arch_get_system_time_offset(sRealTimeData) + system_time();
}


void
set_real_time_clock(uint32 secs)
{
	_kern_set_real_time_clock(secs);
}


status_t
set_timezone(const char* timezone)
{
	char path[B_PATH_NAME_LENGTH];
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, -1, true, path,
		B_PATH_NAME_LENGTH);
	if (status != B_OK) {
		syslog(LOG_ERR, "can't find settings directory: %s\n", strerror(status));
		return status;
	}

	strlcat(path, "/timezone", sizeof(path));

	if (unlink(path) != 0 && errno != ENOENT) {
		syslog(LOG_ERR, "can't unlink: %s\n", strerror(errno));
		return errno;
	}

	if (symlink(timezone, path) != 0) {
		syslog(LOG_ERR, "can't symlink: %s\n", strerror(errno));
		return errno;
	}

	bool isGMT;
	_kern_get_tzfilename(NULL, 0, &isGMT);
	_kern_set_tzfilename(timezone, strlen(timezone), isGMT);

	tzset();

	time_t seconds;
	time(&seconds);
	struct tm tm;
	localtime_r(&seconds, &tm);

	return _kern_set_timezone(tm.tm_gmtoff, tm.tm_isdst);
}


bigtime_t
set_alarm(bigtime_t when, uint32 mode)
{
	return _kern_set_alarm(when, mode);
}
