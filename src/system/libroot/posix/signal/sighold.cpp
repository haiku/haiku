/*
 * Copyright 2007, Vasilis Kaoutsis, kaoutsis@sch.gr
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <symbol_versioning.h>

#include <signal_private.h>


int
__sighold_beos(int signal)
{
	// make an empty set and add the signal
	sigset_t_beos tempSignalSet = 0;
	if (__sigaddset_beos(&tempSignalSet, signal) == -1)
		return -1;

	// add the signal to the calling process' signal mask
	return __pthread_sigmask_beos(SIG_BLOCK, &tempSignalSet, NULL);
}


int
__sighold(int signal)
{
	// make an empty set and add the signal
	sigset_t tempSignalSet = 0;
	if (sigaddset(&tempSignalSet, signal) == -1)
		return -1;

	// add the signal to the calling process' signal mask
	return sigprocmask(SIG_BLOCK, &tempSignalSet, NULL);
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sighold_beos", "sighold@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sighold", "sighold@@", "1_ALPHA4");
