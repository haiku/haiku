/* 
** Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <debug.h>
#include <stdlib.h>

#include <arch/real_time_clock.h>
#include <real_time_clock.h>


static bigtime_t sBootTime;


void
rtc_set_system_time(uint32 current_time)
{
	uint64 useconds;

	useconds = (uint64)current_time * 1000000;
	sBootTime = useconds - system_time();
}


/** Write the system time to CMOS. */

static void
rtc_system_to_hw(void)
{
	uint32 seconds;

	seconds = (sBootTime + system_time()) / 1000000;
	arch_rtc_set_hw_time(seconds);
}


/** Read the CMOS clock and update the system time accordingly. */

static void
rtc_hw_to_system(void)
{
	uint32 current_time;

	current_time = arch_rtc_get_hw_time();
	rtc_set_system_time(current_time);
}


bigtime_t
rtc_boot_time(void)
{
	return sBootTime;
}


static void
rtc_print(void)
{
	uint32 currentTime;

	currentTime = (sBootTime + system_time()) / 1000000;
	dprintf("system_time:  %u\n", (unsigned)system_time());
	dprintf("boot_time:    %u\n", (unsigned)sBootTime);
	dprintf("current_time: %u\n", (unsigned)currentTime);
}


static int
rtc_debug(int argc, char **argv)
{
	if (argc < 2) {
		// If no arguments were given, output all usefull data.
		rtc_print();
	} else {
		// If there was an argument, reset the system and hw time.
		rtc_set_system_time(strtoul(argv[1], NULL, 10));
		rtc_system_to_hw();
	}

	return 0;
}


status_t
rtc_init(kernel_args *ka)
{
	//dprintf("rtc_init: entry\n");
	add_debugger_command("rtc", &rtc_debug, "Set and test the real-time clock");

	rtc_hw_to_system();

	return B_OK;
}

