/* 
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <unistd.h>
#include <syscalls.h>


int
usleep(unsigned useconds)
{
	return snooze_until(system_time() + (bigtime_t)(useconds), B_SYSTEM_TIMEBASE);
}
