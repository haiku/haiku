/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <time.h>
#include <syscalls.h>


time_t
time(time_t *timer)
{
	// ToDo: implement time()

	if (timer)
		*timer = 0;

	return 0;
}

