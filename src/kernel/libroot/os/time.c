/* 
** Copyright 2002-2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include "syscalls.h"


uint32
real_time_clock(void)
{
	// ToDo: real_time_clock()
	return 0;
}


void
set_real_time_clock(uint32 secs)
{
	_kern_set_real_time_clock(secs);
}


bigtime_t
real_time_clock_usecs(void)
{
	// ToDo: real_time_clock_usecs()
	return 0;
}


status_t
set_timezone(char *timezone)
{
	// ToDo: set_timezone()
	return B_ERROR;
}


/*
// ToDo: currently defined in atomic.S - but should be in its own file time.S
bigtime_t
system_time(void)
{
	// time since booting in microseconds
	return sys_system_time();
}
*/


bigtime_t
set_alarm(bigtime_t when, uint32 flags)
{
	// ToDo: set_alarm()
	return B_ERROR;
}
