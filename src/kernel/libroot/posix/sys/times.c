/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <OS.h>

#include <sys/times.h>
#include <time.h>


clock_t
times(struct tms *buffer)
{
	// ToDo: get some real values here!
	buffer->tms_utime = 0;
	buffer->tms_stime = 0;
	buffer->tms_cutime = 0;
	buffer->tms_cstime = 0;

	return real_time_clock() * CLK_TCK;
}

