/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <poll.h>

#include <errno.h>
#include <pthread.h>

#include <syscall_utils.h>

#include <errno_private.h>
#include <syscalls.h>
#include <time_private.h>


extern "C" int __ppoll(struct pollfd *fds, nfds_t numfds, const struct timespec *tv,
	const sigset_t *sigMask);

int
poll(struct pollfd *fds, nfds_t numfds, int timeout)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_poll(fds, numfds, timeout * 1000LL,
		NULL));
}


int
__ppoll(struct pollfd *fds, nfds_t numfds, const struct timespec *tv,
	const sigset_t *sigMask)
{
	int status;
	bigtime_t timeout = -1LL;
	if (tv != NULL && !timespec_to_bigtime(*tv, timeout))
		RETURN_AND_SET_ERRNO_TEST_CANCEL(EINVAL);

	status = _kern_poll(fds, numfds, timeout, sigMask);

	RETURN_AND_SET_ERRNO_TEST_CANCEL(status);
}


B_DEFINE_WEAK_ALIAS(__ppoll, ppoll);
