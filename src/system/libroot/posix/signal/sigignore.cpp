/*
 * Copyright 2007, Vasilis Kaoutsis, kaoutsis@sch.gr
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>


int
sigignore(int signal)
{
	struct sigaction ignoreSignalAction;
		// create an action to ignore the signal

	// request that the signal will be ignored
	// by the handler of the action
	ignoreSignalAction.sa_handler = SIG_IGN;
	ignoreSignalAction.sa_flags = 0;

	// In case of SIGCHLD the specification requires SA_NOCLDWAIT behavior.
	if (signal == SIGCHLD)
		ignoreSignalAction.sa_flags |= SA_NOCLDWAIT;

	return sigaction(signal, &ignoreSignalAction, NULL);
}
