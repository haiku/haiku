/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <syscalls.h>


extern "C" int
siginterrupt(int sig, int flag)
{
	struct sigaction action;

	sigaction(sig, NULL, &action);
	if (flag)
		action.sa_flags &= ~SA_RESTART;
	else
		action.sa_flags |= SA_RESTART;

	return sigaction(sig, &action, NULL);
}

