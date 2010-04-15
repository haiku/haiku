/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <syscalls.h>

#include <signal.h>
#include <errno.h>


int
sigprocmask(int how, const sigset_t *set, sigset_t *oldSet)
{
	int status = _kern_sigprocmask(how, set, oldSet);
	if (status < B_OK) {
		errno = status;
		return -1;
	}

	return 0;
}


int
pthread_sigmask(int how, const sigset_t *set, sigset_t *oldSet)
{
	// TODO: This is obviously broken as sigprocmask() should set the team's
	// signal mask!
	return sigprocmask(how, set, oldSet);
}

