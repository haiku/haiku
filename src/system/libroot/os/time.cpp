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
set_timezone(const char* /*timezone*/)
{
	/* There's nothing we can do here, since we no longer support named
	 * timezones.
	 *
	 * TODO: should we keep this around for compatibility or get rid of it?
	 */
	return B_OK;
}


bigtime_t
set_alarm(bigtime_t when, uint32 mode)
{
	return _kern_set_alarm(when, mode);
}
