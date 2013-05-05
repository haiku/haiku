/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <signal.h>

#include <errno_private.h>
#include <syscalls.h>


int
sigaltstack(const stack_t *alternateStack, stack_t *oldAlternateStack)
{
	status_t status =_kern_set_signal_stack(alternateStack, oldAlternateStack);
	if (status < B_OK) {
		__set_errno(status);
		return -1;
	}

	return 0;
}
