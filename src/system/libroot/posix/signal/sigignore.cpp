/*
 * Copyright 2007, Vasilis Kaoutsis, kaoutsis@sch.gr
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <symbol_versioning.h>

#include <signal_private.h>


int
__sigignore_beos(int signal)
{
	// create an action to ignore the signal
	struct sigaction_beos ignoreSignalAction;
	ignoreSignalAction.sa_handler = SIG_IGN;
	ignoreSignalAction.sa_flags = 0;

	// In case of SIGCHLD the specification requires SA_NOCLDWAIT behavior.
	if (signal == SIGCHLD)
		ignoreSignalAction.sa_flags |= SA_NOCLDWAIT;

	return __sigaction_beos(signal, &ignoreSignalAction, NULL);
}


int
__sigignore(int signal)
{
	// create an action to ignore the signal
	struct sigaction ignoreSignalAction;
	ignoreSignalAction.sa_handler = SIG_IGN;
	ignoreSignalAction.sa_flags = 0;

	// In case of SIGCHLD the specification requires SA_NOCLDWAIT behavior.
	if (signal == SIGCHLD)
		ignoreSignalAction.sa_flags |= SA_NOCLDWAIT;

	return sigaction(signal, &ignoreSignalAction, NULL);
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigignore_beos", "sigignore@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigignore", "sigignore@@", "1_ALPHA4");
