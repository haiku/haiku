/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <time.h>
#include <OS.h>

#include <symbol_versioning.h>

#include <time_private.h>


clock_t
__clock_beos(void)
{
	thread_info info;
	get_thread_info(find_thread(NULL), &info);

	return (clock_t)((info.kernel_time + info.user_time)
		/ MICROSECONDS_PER_CLOCK_TICK_BEOS);
}


clock_t
__clock(void)
{
	thread_info info;
	get_thread_info(find_thread(NULL), &info);

	return (clock_t)((info.kernel_time + info.user_time)
		/ MICROSECONDS_PER_CLOCK_TICK);
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__clock_beos", "clock@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__clock", "clock@@", "1_ALPHA4");
