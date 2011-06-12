/*
 * Copyright 2006-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_REAL_TIME_CLOCK_H
#define _KERNEL_REAL_TIME_CLOCK_H


#include <KernelExport.h>

#include <time.h>


struct kernel_args;


#define RTC_EPOCH_BASE_YEAR	1970

#ifdef __cplusplus
extern "C" {
#endif

void set_real_time_clock_usecs(bigtime_t currentTime);

status_t rtc_init(struct kernel_args *args);
bigtime_t rtc_boot_time(void);
	// Returns the time at which the system was booted in microseconds since Jan 1, 1970 UTC.

// Both functions use the passed struct tm only partially
// (no tm_wday, tm_yday, tm_isdst).
uint32 rtc_tm_to_secs(const struct tm *t);
void rtc_secs_to_tm(uint32 seconds, struct tm *t);

uint32 get_timezone_offset(void);

bigtime_t _user_system_time(void);
status_t _user_set_real_time_clock(bigtime_t time);
status_t _user_set_timezone(int32 timezoneOffset, const char *name,
			size_t nameLength);
status_t _user_get_timezone(int32 *_timezoneOffset, char* name,
			size_t nameLength);
status_t _user_set_real_time_clock_is_gmt(bool isGMT);
status_t _user_get_real_time_clock_is_gmt(bool *_isGMT);

#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_REAL_TIME_CLOCK_H */
