/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <poll.h>
#include <syscalls.h>


int
poll(struct pollfd *fds, nfds_t numfds, int timeout)
{
	return _kern_poll(fds, numfds, timeout * 1000LL);
}

