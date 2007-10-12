/* 
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the OpenBeOS License.
 */


#include <errno.h>
#include <poll.h>
#include <syscalls.h>


int
poll(struct pollfd *fds, nfds_t numfds, int timeout)
{
	int result = _kern_poll(fds, numfds, timeout * 1000LL);
	if (result < 0) {
		errno = result;
		return -1;
	}
	return result;
}
