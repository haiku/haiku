/*
 * Copyright 2008, Vasilis Kaoutsis, kaoutsis@sch.gr
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>


int
sigpause(int signal)
{
	sigset_t processSignalSet;

	if (sigprocmask(SIG_SETMASK, NULL, &processSignalSet) == -1)
		return -1;

	if (sigdelset(&processSignalSet, signal) == -1)
		return -1;

	return sigsuspend(&processSignalSet);
}
