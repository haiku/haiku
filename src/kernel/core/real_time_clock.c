/* 
** Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <debug.h>
#include <stdlib.h>

#include <arch/real_time_clock.h>
#include <real_time_clock.h>

#define RTC_REGION_SIZE	B_PAGE_SIZE

static bigtime_t *sBootTime;
static region_id sRtcArea;


/** Write the system time to CMOS. */

static void
rtc_system_to_hw(void)
{
	uint32 seconds;

	seconds = (*sBootTime + system_time()) / 1000000;
	arch_rtc_set_hw_time(seconds);
}


/** Read the CMOS clock and update the system time accordingly. */

static void
rtc_hw_to_system(void)
{
	uint32 current_time;

	current_time = arch_rtc_get_hw_time();
	set_real_time_clock(current_time);
}


bigtime_t
rtc_boot_time(void)
{
	return *sBootTime;
}


static void
rtc_print(void)
{
	uint32 currentTime;

	currentTime = (*sBootTime + system_time()) / 1000000;
	dprintf("system_time:  %Ld\n", system_time());
	dprintf("boot_time:    %Ld\n", *sBootTime);
	dprintf("current_time: %lu\n", currentTime);
}


static int
rtc_debug(int argc, char **argv)
{
	if (argc < 2) {
		// If no arguments were given, output all usefull data.
		rtc_print();
	} else {
		// If there was an argument, reset the system and hw time.
		set_real_time_clock(strtoul(argv[1], NULL, 10));
	}

	return 0;
}


status_t
rtc_init(kernel_args *ka)
{
	//dprintf("rtc_init: entry\n");
	add_debugger_command("rtc", &rtc_debug, "Set and test the real-time clock");

	sRtcArea = create_area("rtc_region", (void**)&sBootTime, B_ANY_KERNEL_ADDRESS,
		RTC_REGION_SIZE, B_NO_LOCK, B_READ_AREA);
	if (sRtcArea < 0) {
		panic("rtc_init: error creating rtc region\n");
		return B_NO_MEMORY;
	}

	rtc_hw_to_system();

	return B_OK;
}


//	#pragma mark -
//	public kernel API


void
set_real_time_clock(uint32 currentTime)
{
	*sBootTime = currentTime * 1000000LL - system_time();
	rtc_system_to_hw();
}


uint32
real_time_clock(void)
{
	// ToDo: implement me - they might be used directly from libroot/os/time.c
	return (*sBootTime + system_time()) / 1000000;
}


bigtime_t
real_time_clock_usecs(void)
{
	// ToDo: implement me - they might be used directly from libroot/os/time.c
	return *sBootTime + system_time();
}


//	#pragma mark -
//	public userland API


status_t
_user_set_real_time_clock(uint32 time)
{
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	set_real_time_clock(time);
	return B_OK;
}

