/* 
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <unistd.h>
#include <syscalls.h>


int
usleep(unsigned useconds)
{
	return sys_snooze((bigtime_t)(useconds));
}
