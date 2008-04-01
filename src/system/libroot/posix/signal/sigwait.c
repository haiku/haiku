/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <syscalls.h>

#include <signal.h>


int
sigwait(const sigset_t *set, int *_signal)
{
	return _kern_sigwait(set, _signal);
		// does not set errno, but returns the error value directly
}

