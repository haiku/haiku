/* 
** Copyright 2007, Jérôme Duval. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#include <errno.h>
#include <time.h>
#include <OS.h>

int
nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	bigtime_t ms, begin_ms;
	status_t err;

	if (!rqtp) {
		errno = EINVAL;
		return -1;
	}

	// round up tv_nsec
	ms = rqtp->tv_sec * 1000000LL + (rqtp->tv_nsec + 999) / 1000;
	begin_ms = system_time();
	err = snooze(ms);
	if (err == B_INTERRUPTED) {
		errno = EINTR;
		if (rmtp) {
			bigtime_t remain_ms = ms - system_time() + begin_ms;
			rmtp->tv_sec = remain_ms / 1000000;
			rmtp->tv_nsec = (remain_ms - rmtp->tv_sec) * 1000;
		}
		return -1;
	}
	return 0;
}

