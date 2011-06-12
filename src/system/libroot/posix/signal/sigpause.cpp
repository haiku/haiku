/*
 * Copyright 2008, Vasilis Kaoutsis, kaoutsis@sch.gr
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <symbol_versioning.h>

#include <signal_private.h>


int
__sigpause_beos(int signal)
{
	sigset_t_beos processSignalSet;
	if (__pthread_sigmask_beos(SIG_SETMASK, NULL, &processSignalSet) == -1)
		return -1;

	if (__sigdelset_beos(&processSignalSet, signal) == -1)
		return -1;

	return __sigsuspend_beos(&processSignalSet);
}


int
__sigpause(int signal)
{
	sigset_t processSignalSet;
	if (sigprocmask(SIG_SETMASK, NULL, &processSignalSet) == -1)
		return -1;

	if (sigdelset(&processSignalSet, signal) == -1)
		return -1;

	return sigsuspend(&processSignalSet);
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigpause_beos", "sigpause@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigpause", "sigpause@@", "1_ALPHA4");
