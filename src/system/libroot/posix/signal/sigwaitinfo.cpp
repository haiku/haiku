/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>


int
sigwaitinfo(const sigset_t* set, siginfo_t* info)
{
	return sigtimedwait(set, info, NULL);
}
