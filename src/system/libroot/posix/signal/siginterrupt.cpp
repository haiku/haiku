/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <symbol_versioning.h>

#include <signal_private.h>


int
__siginterrupt_beos(int signal, int flag)
{
	struct sigaction_beos action;
	__sigaction_beos(signal, NULL, &action);
	if (flag)
		action.sa_flags &= ~SA_RESTART;
	else
		action.sa_flags |= SA_RESTART;

	return __sigaction_beos(signal, &action, NULL);
}


int
__siginterrupt(int signal, int flag)
{
	struct sigaction action;
	sigaction(signal, NULL, &action);
	if (flag)
		action.sa_flags &= ~SA_RESTART;
	else
		action.sa_flags |= SA_RESTART;

	return sigaction(signal, &action, NULL);
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__siginterrupt_beos", "siginterrupt@",
	"BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__siginterrupt", "siginterrupt@@",
	"1_ALPHA4");
