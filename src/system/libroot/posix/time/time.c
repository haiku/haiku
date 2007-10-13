/* 
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <time.h>
#include <OS.h>


time_t
time(time_t* timer)
{
	time_t secs = real_time_clock();

	if (timer)
		*timer = secs;

	return secs;
}
