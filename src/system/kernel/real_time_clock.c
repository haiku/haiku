/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <arch/real_time_clock.h>
#include <real_time_clock.h>
#include <real_time_data.h>
#include <syscalls.h>

#include <stdlib.h>

//#define TRACE_TIME
#ifdef TRACE_TIME
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


static struct real_time_data *sRealTimeData;
static bool sIsGMT = false;
static char sTimezoneFilename[B_PATH_NAME_LENGTH] = "";
static bigtime_t sTimezoneOffset = 0;
static bool sDaylightSavingTime = false;


/** Write the system time to CMOS. */

static void
rtc_system_to_hw(void)
{
	uint32 seconds;

	seconds = (arch_rtc_get_system_time_offset(sRealTimeData) + system_time() 
		+ (sIsGMT ? 0 : sTimezoneOffset)) / 1000000;
	
	arch_rtc_set_hw_time(seconds);
}


/** Read the CMOS clock and update the system time accordingly. */

static void
rtc_hw_to_system(void)
{
	uint32 current_time;

	current_time = arch_rtc_get_hw_time();
	set_real_time_clock(current_time + (sIsGMT ? 0 : sTimezoneOffset));
}


bigtime_t
rtc_boot_time(void)
{
	return arch_rtc_get_system_time_offset(sRealTimeData);
}


static int
rtc_debug(int argc, char **argv)
{
	if (argc < 2) {
		// If no arguments were given, output all useful data.
		uint32 currentTime;
		bigtime_t systemTimeOffset
			= arch_rtc_get_system_time_offset(sRealTimeData);
	
		currentTime = (systemTimeOffset + system_time()) / 1000000;
		dprintf("system_time:  %Ld\n", system_time());
		dprintf("system_time_offset:    %Ld\n", systemTimeOffset);
		dprintf("current_time: %lu\n", currentTime);
	} else {
		// If there was an argument, reset the system and hw time.
		set_real_time_clock(strtoul(argv[1], NULL, 10));
	}

	return 0;
}


status_t
rtc_init(kernel_args *args)
{
	void *clonedRealTimeData;

	area_id area = create_area("real time data", (void **)&sRealTimeData, B_ANY_KERNEL_ADDRESS,
		PAGE_ALIGN(sizeof(struct real_time_data)), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK) {
		panic("rtc_init: error creating real time data area\n");
		return area;
	}

	// On some systems like x86, a page cannot be read-only in userland and writable
	// in the kernel. Therefore, we clone the real time data area here for user
	// access; it doesn't hurt on other platforms, too.
	// The area is used to share time critical information, such as the system
	// time conversion factor which can change at any time.

	if (clone_area("real time data userland", &clonedRealTimeData, B_ANY_KERNEL_ADDRESS, 
		B_READ_AREA, area) < B_OK) {
		dprintf("rtc_init: error creating real time data userland area\n");
		// we don't panic because it's not kernel critical
	}

	arch_rtc_init(args, sRealTimeData);
	rtc_hw_to_system();

	add_debugger_command("rtc", &rtc_debug, "Set and test the real-time clock");
	return B_OK;
}


//	#pragma mark - public kernel API


void
set_real_time_clock(uint32 currentTime)
{
	arch_rtc_set_system_time_offset(sRealTimeData,
		currentTime * 1000000LL - system_time());
	rtc_system_to_hw();
}


uint32
real_time_clock(void)
{
	return (arch_rtc_get_system_time_offset(sRealTimeData) + system_time())
		/ 1000000;
}


bigtime_t
real_time_clock_usecs(void)
{
	return arch_rtc_get_system_time_offset(sRealTimeData) + system_time();
}


status_t
get_rtc_info(rtc_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;
	info->time = real_time_clock();
	info->is_gmt = sIsGMT;
	info->tz_minuteswest = sTimezoneOffset / 1000000LL;
	info->tz_dsttime = sDaylightSavingTime;

	return B_OK;
}


// #pragma mark -


#define SECONDS_31 2678400
#define SECONDS_30 2592000
#define SECONDS_28 2419200
#define SECONDS_DAY 86400

static uint32 sSecsPerMonth[12] = {SECONDS_31, SECONDS_28, SECONDS_31, SECONDS_30, 
	SECONDS_31, SECONDS_30, SECONDS_31, SECONDS_31, SECONDS_30, SECONDS_31, SECONDS_30,
	SECONDS_31};


static bool
leap_year(uint32 year)
{
	if (year % 400 == 0)
		return true;

	if (year % 100 == 0)
		return false;

	if (year % 4 == 0)
		return true;

	return false;
}


static inline uint32
secs_this_year(uint32 year)
{
	if (leap_year(year))
		return 31622400;
	
	return 31536000;
}


uint32
rtc_tm_to_secs(const struct tm *t)
{
	uint32 wholeYear;
	uint32 time = 0;
	uint32 i;

	wholeYear = RTC_EPOCHE_BASE_YEAR + t->tm_year;

	// ToDo: get rid of these loops and compute the correct value
	//	i.e. days = (long)(year > 0) + year*365 + --year/4 - year/100 + year/400;
	//	let sSecsPerMonth[] have the values already added up

	// Add up the seconds from all years since 1970 that have elapsed.
	for (i = RTC_EPOCHE_BASE_YEAR; i < wholeYear; ++i) {
		time += secs_this_year(i);
	}

	// Add up the seconds from all months passed this year.
	for (i = 0; i < t->tm_mon && i < 12; ++i)
		time += sSecsPerMonth[i];

	// Add up the seconds from all days passed this month.
	if (leap_year(wholeYear) && t->tm_mon >= 2)
		time += SECONDS_DAY;

	time += (t->tm_mday - 1) * SECONDS_DAY;
	time += t->tm_hour * 3600;
	time += t->tm_min * 60;
	time += t->tm_sec;

	return time;
}


void
rtc_secs_to_tm(uint32 seconds, struct tm *t)
{
	uint32 wholeYear = RTC_EPOCHE_BASE_YEAR;
	uint32 secsThisYear;
	bool keepLooping;
	bool isLeapYear;
	int temp;
	int month;

	keepLooping = 1;

	// Determine the current year by starting at 1970 and incrementing whole_year as long as
	// we can keep subtracting secs_this_year from seconds.
	while (keepLooping) {
		secsThisYear = secs_this_year(wholeYear);

		if (seconds >= secsThisYear) {
			seconds -= secsThisYear;
			++wholeYear;
		} else
			keepLooping = false;
	}

	t->tm_year = wholeYear - RTC_EPOCHE_BASE_YEAR;

	// Determine the current month
	month = 0;
	isLeapYear = leap_year(wholeYear);
	do {
		temp = seconds - sSecsPerMonth[month];

		if (isLeapYear && month == 1)
			temp -= SECONDS_DAY;

		if (temp >= 0) {
			seconds = temp;
			++month;
		}
	} while (temp >= 0 && month < 12);

	t->tm_mon = month;

	t->tm_mday = seconds / SECONDS_DAY + 1;
	seconds = seconds % SECONDS_DAY;

	t->tm_hour = seconds / 3600;
	seconds = seconds % 3600;

	t->tm_min = seconds / 60;
	seconds = seconds % 60;

	t->tm_sec = seconds;
}


//	#pragma mark -


/**	This is called from the gettimeofday() implementation that's part of the
 *	kernel.
 */

status_t
_kern_get_timezone(time_t *_timezoneOffset, bool *_daylightSavingTime)
{
	*_timezoneOffset = (time_t)(sTimezoneOffset / 1000000LL);
	*_daylightSavingTime = sDaylightSavingTime;

	return B_OK;
}


//	#pragma mark - syscalls


bigtime_t
_user_system_time(void)
{
	return system_time();
}


status_t
_user_set_real_time_clock(uint32 time)
{
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	set_real_time_clock(time);
	return B_OK;
}


status_t
_user_set_timezone(time_t timezoneOffset, bool daylightSavingTime)
{
	bigtime_t offset = (bigtime_t)timezoneOffset * 1000000LL;

	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	TRACE(("old system_time_offset %Ld old %Ld new %Ld gmt %d\n",
		arch_rtc_get_system_time_offset(sRealTimeData), sTimezoneOffset, offset,
		sIsGMT));

	// We only need to update our time offset if the hardware clock
	// does not run in the local timezone.
	// Since this is shared data, we need to update it atomically.
	if (!sIsGMT) {
		arch_rtc_set_system_time_offset(sRealTimeData,
			arch_rtc_get_system_time_offset(sRealTimeData) + sTimezoneOffset - offset);
	}

	sTimezoneOffset = offset;
	sDaylightSavingTime = daylightSavingTime;

	TRACE(("new system_time_offset %Ld\n",
		arch_rtc_get_system_time_offset(sRealTimeData)));

	return B_OK;
}


status_t
_user_get_timezone(time_t *_timezoneOffset, bool *_daylightSavingTime)
{
	time_t offset = (time_t)(sTimezoneOffset / 1000000LL);

	if (!IS_USER_ADDRESS(_timezoneOffset) || !IS_USER_ADDRESS(_daylightSavingTime)
		|| user_memcpy(_timezoneOffset, &offset, sizeof(time_t)) < B_OK
		|| user_memcpy(_daylightSavingTime, &sDaylightSavingTime, sizeof(bool)) < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


status_t
_user_set_tzfilename(const char *filename, size_t length, bool isGMT)
{
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	if (!IS_USER_ADDRESS(filename)
		|| filename == NULL
		|| user_strlcpy(sTimezoneFilename, filename, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	// ToDo: Shouldn't this update the system_time_offset as well?
	sIsGMT = isGMT;
	return B_OK;
}


status_t
_user_get_tzfilename(char *filename, size_t length, bool *_isGMT)
{
	if (filename == NULL || _isGMT == NULL
		|| !IS_USER_ADDRESS(filename) || !IS_USER_ADDRESS(_isGMT)
		|| user_strlcpy(filename, sTimezoneFilename, length) < B_OK
		|| user_memcpy(_isGMT, &sIsGMT, sizeof(bool)) < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}

