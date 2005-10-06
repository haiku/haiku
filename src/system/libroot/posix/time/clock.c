/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <time.h>
#include <OS.h>


clock_t
clock(void)
{
	thread_info info;
	get_thread_info(find_thread(NULL), &info);

	return (clock_t)((info.kernel_time + info.user_time) / 1000);
		// unlike the XSI specs say, CLOCKS_PER_SEC is 1000 and not 1000000 in
		// BeOS - that means we have to convert the bigtime_t to CLOCKS_PER_SEC
		// before we return it
}

