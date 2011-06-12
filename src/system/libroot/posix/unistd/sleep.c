/*
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <unistd.h>

#include <pthread.h>

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

	pthread_testcancel();

	if (err)
		return seconds - (unsigned)((system_time() - start) / 1000000);

	return 0;
}
