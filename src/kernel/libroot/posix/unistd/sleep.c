/* 
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <unistd.h>
#include <syscalls.h>


unsigned
sleep(unsigned seconds)
{
	bigtime_t usecs;
	bigtime_t start;
	int err;
	unsigned retval;

	start= sys_system_time();

	usecs= 1000000;
	usecs*= (bigtime_t) seconds;

	err= sys_snooze(usecs);

	retval= 0;
	if(err) {
		retval= (unsigned)(sys_system_time()-start);
	}

	return retval;
}
