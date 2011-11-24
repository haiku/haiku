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


int
poll(struct pollfd *fds, nfds_t numfds, int timeout)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_poll(fds, numfds, timeout * 1000LL));
}
