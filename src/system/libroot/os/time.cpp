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
#include <user_timer_defs.h>
#include <syscalls.h>


static struct real_time_data* sRealTimeData;


void
__init_time(void)
{
	sRealTimeData = (struct real_time_data*)
		USER_COMMPAGE_TABLE[COMMPAGE_ENTRY_REAL_TIME_DATA];

	__arch_init_time(sRealTimeData, false);
}


bigtime_t
__get_system_time_offset()
{
	return __arch_get_system_time_offset(sRealTimeData);
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
	_kern_set_real_time_clock((bigtime_t)secs * 1000000);
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
	// prepare the values to be passed to the kernel
	bigtime_t interval = 0;
	uint32 flags = B_RELATIVE_TIMEOUT;

	if (when == B_INFINITE_TIMEOUT) {
		when = B_INFINITE_TIMEOUT;
	} else {
		switch (mode) {
			case B_PERIODIC_ALARM:
				interval = when;
				break;
			case B_ONE_SHOT_ABSOLUTE_ALARM:
				flags = B_ABSOLUTE_TIMEOUT;
				break;
			case B_ONE_SHOT_RELATIVE_ALARM:
			default:
				break;
		}
	}

	// set the timer
	user_timer_info oldInfo;
	status_t error = _kern_set_timer(USER_TIMER_REAL_TIME_ID, find_thread(NULL),
		when, interval, flags, &oldInfo);
	if (error != B_OK)
		return 0;

	// A remaining time of B_INFINITE_TIMEOUT means not scheduled.
	return oldInfo.remaining_time != B_INFINITE_TIMEOUT
		? oldInfo.remaining_time : 0;
}
