/* 
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <arch/real_time_clock.h>
#include <real_time_clock.h>
#include <real_time_data.h>

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

	seconds = (sRealTimeData->system_time_offset + system_time() 
		- (sIsGMT ? 0 : sTimezoneOffset)) / 1000000;
	
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
	return sRealTimeData->system_time_offset;
}


static int
rtc_debug(int argc, char **argv)
{
	if (argc < 2) {
		// If no arguments were given, output all useful data.
		uint32 currentTime;
	
		currentTime = (sRealTimeData->system_time_offset + system_time()) / 1000000;
		dprintf("system_time:  %Ld\n", system_time());
		dprintf("system_time_offset:    %Ld\n", sRealTimeData->system_time_offset);
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


//	#pragma mark -
//	public kernel API


void
set_real_time_clock(uint32 currentTime)
{
	atomic_set64(&sRealTimeData->system_time_offset, currentTime * 1000000LL - system_time());
	rtc_system_to_hw();
}


uint32
real_time_clock(void)
{
	return (sRealTimeData->system_time_offset + system_time()) / 1000000;
}


bigtime_t
real_time_clock_usecs(void)
{
	return sRealTimeData->system_time_offset + system_time();
}


status_t
get_rtc_info(rtc_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;
	info->time = real_time_clock();
	info->is_gmt = sIsGMT;
	info->tz_minuteswest = sTimezoneOffset;
	info->tz_dsttime = sDaylightSavingTime;

	return B_OK;
}


//	#pragma mark -
//	public userland API


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

	TRACE(("old system_time_offset %Ld old %Ld new %Ld gmt %d\n", sRealTimeData->system_time_offset, sTimezoneOffset, offset, sIsGMT));

	// We only need to update our time offset if the hardware clock
	// does not run in the local timezone.
	// Since this is shared data, we need to update it atomically.
	if (!sIsGMT)
		atomic_add64(&sRealTimeData->system_time_offset, sTimezoneOffset - offset);

	sTimezoneOffset = offset;
	sDaylightSavingTime = daylightSavingTime;

	TRACE(("new system_time_offset %Ld\n", sRealTimeData->system_time_offset));

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

