/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <sys/select.h>
#include <syscalls.h>


int 
pselect(int numBits, struct fd_set *readBits, struct fd_set *writeBits,
	struct fd_set *errorBits, const struct timespec *tv, const sigset_t *sigMask)
{
	bigtime_t timeout = -1LL;
	if (tv)
		timeout = tv->tv_sec * 1000000LL + tv->tv_nsec / 1000LL;

	return _kern_select(numBits, readBits, writeBits, errorBits, timeout, sigMask);
}


int 
select(int numBits, struct fd_set *readBits, struct fd_set *writeBits,
	struct fd_set *errorBits, struct timeval *tv)
{
	bigtime_t timeout = -1LL;
	if (tv)
		timeout = tv->tv_sec * 1000000LL + tv->tv_usec;

	return _kern_select(numBits, readBits, writeBits, errorBits, timeout, NULL);
}

