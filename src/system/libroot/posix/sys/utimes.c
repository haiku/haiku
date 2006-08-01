/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <sys/time.h>
#include <utime.h>


int
utimes(const char *file, const struct timeval times[2])
{
	struct utimbuf buffer, *timeBuffer;

	if (times != NULL) {
		timeBuffer = &buffer;
		buffer.actime = times[0].tv_sec + times[0].tv_usec / 1000000LL;
		buffer.modtime = times[1].tv_sec + times[1].tv_usec / 1000000LL;
	} else
		timeBuffer = NULL;

	return utime(file, timeBuffer);
}

