/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <KernelExport.h>

#include <arch/real_time_clock.h>
#include <real_time_clock.h>
#include <real_time_data.h>
#include <vm_types.h>

#include <stdlib.h>


static struct real_time_data *sRealTimeData;


/** Write the system time to CMOS. */

static void
rtc_system_to_hw(void)
{
	uint32 seconds;

	seconds = (sRealTimeData->boot_time + system_time()) / 1000000;
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
	return sRealTimeData->boot_time;
}


static void
rtc_print(void)
{
	uint32 currentTime;

	currentTime = (sRealTimeData->boot_time + system_time()) / 1000000;
	dprintf("system_time:  %Ld\n", system_time());
	dprintf("boot_time:    %Ld\n", sRealTimeData->boot_time);
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
	void *clonedRealTimeData;
	area_id area = create_area("real time data", (void **)&sRealTimeData, B_ANY_KERNEL_ADDRESS,
		PAGE_ALIGN(sizeof(struct real_time_data)), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK) {
		panic("rtc_init: error creating real time data area\n");
		return area;
	}

	if (clone_area("real time data userland", &clonedRealTimeData, B_CLONE_ADDRESS, 
		B_READ_AREA, area) < B_OK) {
		dprintf("rtc_init: error creating real time data userland area\n");
		// we don't panic because it's not kernel critical
	}

	rtc_hw_to_system();

	add_debugger_command("rtc", &rtc_debug, "Set and test the real-time clock");
	return B_OK;
}


//	#pragma mark -
//	public kernel API


void
set_real_time_clock(uint32 currentTime)
{
	sRealTimeData->boot_time = currentTime * 1000000LL - system_time();
	rtc_system_to_hw();
}


uint32
real_time_clock(void)
{
	// ToDo: implement me - they might be used directly from libroot/os/time.c
	return (sRealTimeData->boot_time + system_time()) / 1000000;
}


bigtime_t
real_time_clock_usecs(void)
{
	// ToDo: implement me - they might be used directly from libroot/os/time.c
	return sRealTimeData->boot_time + system_time();
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

