/* 
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <sys/select.h>
#include <syscalls.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int 
pselect(int numBits, struct fd_set *readBits, struct fd_set *writeBits,
	struct fd_set *errorBits, const struct timespec *tv, const sigset_t *sigMask)
{
	int status;
	bigtime_t timeout = -1LL;
	if (tv)
		timeout = tv->tv_sec * 1000000LL + tv->tv_nsec / 1000LL;

	status = _kern_select(numBits, readBits, writeBits, errorBits, timeout, sigMask);

	RETURN_AND_SET_ERRNO(status);
}


int 
select(int numBits, struct fd_set *readBits, struct fd_set *writeBits,
	struct fd_set *errorBits, struct timeval *tv)
{
	int status;
	bigtime_t timeout = -1LL;
	if (tv)
		timeout = tv->tv_sec * 1000000LL + tv->tv_usec;

	status = _kern_select(numBits, readBits, writeBits, errorBits, timeout, NULL);

	RETURN_AND_SET_ERRNO(status);
}
