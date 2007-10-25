/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Daniel Reinhold (danielre@users.sf.net)
 */


#include <errno.h>
#include <signal.h>
#include <syscalls.h>


int
sigemptyset(sigset_t *set)	
{
	*set = (sigset_t) 0L;
	return 0;
}


int
sigfillset(sigset_t *set)
{
	*set = (sigset_t) ~(0UL);
	return 0;
}


int
sigismember(const sigset_t *set, int sig)
{
	sigset_t mask;

	if (sig <= 0 || sig >= NSIG) {
			errno = EINVAL;
			return -1;
	}

	mask = (((sigset_t)1) << (sig - 1)) ;
	return (*set & mask) ? 1 : 0 ;
}


int
sigaddset(sigset_t *set, int sig)
{
	sigset_t mask;

	if (sig <= 0 || sig >= NSIG) {
			errno = EINVAL;
			return -1;
	}

	mask = (((sigset_t)1) << (sig - 1)) ;
	*set |= mask;
	return 0;
}


int
sigdelset(sigset_t *set, int sig)
{
	sigset_t mask;

	if (sig <= 0 || sig >= NSIG) {
			errno = EINVAL;
			return -1;
	}

	mask = (((sigset_t)1) << (sig - 1)) ;
	*set &= ~mask;
	return 0;
}
