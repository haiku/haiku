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

	start = system_time();

	usecs = 1000000;
	usecs *= (bigtime_t) seconds;

	err = snooze_until(start + usecs, B_SYSTEM_TIMEBASE);
	if (err)
		return (unsigned)(system_time() - start);

	return 0;
}
