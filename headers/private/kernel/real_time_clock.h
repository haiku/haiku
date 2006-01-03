/* 
** Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef _KERNEL_REAL_TIME_CLOCK_H
#define _KERNEL_REAL_TIME_CLOCK_H


#include <KernelExport.h>

#include <time.h>


#define RTC_EPOCHE_BASE_YEAR	1970

#ifdef __cplusplus
extern "C" {
#endif

status_t rtc_init(kernel_args *args);
bigtime_t rtc_boot_time(void);
	// Returns the time at which the system was booted in microseconds since Jan 1, 1970 UTC.

typedef struct rtc_info {
	uint32 time;
	bool is_gmt;
	int32 tz_minuteswest;
	bool tz_dsttime;
} rtc_info;

status_t get_rtc_info(rtc_info *info);

// Both functions use the passed struct tm only partially
// (no tm_wday, tm_yday, tm_isdst).
uint32 rtc_tm_to_secs(const struct tm *t);
void rtc_secs_to_tm(uint32 seconds, struct tm *t);

bigtime_t _user_system_time(void);
status_t _user_set_real_time_clock(uint32 time);
status_t _user_set_timezone(int32 timezoneOffset, bool daylightSavingTime);
status_t _user_set_tzfilename(const char* filename, size_t length, bool isGMT);
status_t _user_get_tzfilename(char *filename, size_t length, bool *_isGMT);

#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_REAL_TIME_CLOCK_H */
