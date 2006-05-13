/* 
 * Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <FindDirectory.h>
#include <OS.h>

#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <libroot_private.h>
#include <real_time_data.h>
#include <syscalls.h>


static struct real_time_data sRealTimeDefaults;
static struct real_time_data *sRealTimeData;


void
__init_time(void)
{
	bool setDefaults = false;
	area_id dataArea; 
	area_info info;

	dataArea = find_area("real time data userland");
	if (dataArea < 0 || get_area_info(dataArea, &info) < B_OK) {
		syslog(LOG_ERR, "error finding real time data area: %s\n", strerror(dataArea));
		sRealTimeData = &sRealTimeDefaults;
		setDefaults = true;
	} else
		sRealTimeData = (struct real_time_data *)info.address;

	__arch_init_time(sRealTimeData, setDefaults);
}


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
set_timezone(char *timezone)
{
	char path[B_PATH_NAME_LENGTH];
	char tzfilename[B_PATH_NAME_LENGTH];
	bool isGMT;

	status_t err;
	struct tm *tm;
	time_t t;
	
	if ((err = find_directory(B_USER_SETTINGS_DIRECTORY, -1, true, path, B_PATH_NAME_LENGTH)) != B_OK) {
		fprintf(stderr, "can't find settings directory: %s\n", strerror(err));
		return err;
	}
	strcat(path, "/timezone");

	err = unlink(path);
	if (err != B_OK) {
		fprintf(stderr, "can't unlink: %s\n", strerror(err));
		return err;
	}
	err = symlink(timezone, path);
	if (err != B_OK) {
		fprintf(stderr, "can't symlink: %s\n", strerror(err));
		return err;
	}
	_kern_get_tzfilename(tzfilename, sizeof(tzfilename), &isGMT);
	_kern_set_tzfilename(timezone, strlen(timezone), isGMT);
	tzset();

	time(&t);
	tm = localtime(&t);

	if ((err = _kern_set_timezone(tm->tm_gmtoff, tm->tm_isdst)) < B_OK)
		return err;

	return B_OK;
}


bigtime_t
set_alarm(bigtime_t when, uint32 flags)
{
	// ToDo: set_alarm()
	return B_ERROR;
}
