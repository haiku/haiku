/* 
** Copyright 2002-2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <string.h>
#include "syscalls.h"
#include "real_time_data.h"

static volatile bigtime_t *sBootTime = NULL;

static status_t
setup_rtc_boottime()
{
	area_id dataArea; 
	area_info info;
	status_t err;

	dataArea = find_area("real time data userland");
	
	if (dataArea < 0) {
		printf("setup_rtc_boottime: error finding real time data area %s\n",
			strerror(dataArea));
		return dataArea;
	}

	err = get_area_info(dataArea, &info);
	if (err < B_OK) {
		printf("setup_rtc_boottime: error getting real time data info\n");
		return err;
	}
	sBootTime = &((struct real_time_data *)info.address)->boot_time;
	return B_OK;
}


uint32
real_time_clock(void)
{
	if (!sBootTime && (setup_rtc_boottime()!=B_OK))
		return 0;
	return (*sBootTime + system_time()) / 1000000;
}


void
set_real_time_clock(uint32 secs)
{
	_kern_set_real_time_clock(secs);
}


bigtime_t
real_time_clock_usecs(void)
{
	if (!sBootTime && (setup_rtc_boottime() != B_OK))
		return 0;
	return *sBootTime + system_time();
}


status_t
set_timezone(char *timezone)
{
	// ToDo: set_timezone()
	return B_ERROR;
}


/*
// ToDo: currently defined in atomic.S - but should be in its own file time.S
bigtime_t
system_time(void)
{
	// time since booting in microseconds
	return sys_system_time();
}
*/


bigtime_t
set_alarm(bigtime_t when, uint32 flags)
{
	// ToDo: set_alarm()
	return B_ERROR;
}
