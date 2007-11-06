/*
 * Copyright 2007, Vasilis Kaoutsis, kaoutsis@sch.gr
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>


int
sighold(int signal)
{
	// make an empty set
	// and add the signal
	sigset_t tempSignalSet;
	sigemptyset(&tempSignalSet);
	if (sigaddset(&tempSignalSet, signal) == -1)
		return -1;
	
	// add the signal to the calling process' signal mask
	return sigprocmask(SIG_BLOCK, &tempSignalSet, NULL);
}
