/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <signal.h>


int
sigsetmask(int mask)
{
	sigset_t set = mask;
	sigset_t oset;

	if (sigprocmask(SIG_SETMASK, &set, &oset) < 0)
		return -1;

	return (int)oset;
}


int
sigblock(int mask)
{
	sigset_t set = mask;
	sigset_t oset;

	if (sigprocmask(SIG_BLOCK, &set, &oset) < 0)
		return -1;

	return (int)oset;
}

