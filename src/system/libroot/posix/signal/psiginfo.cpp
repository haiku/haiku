/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>


void
psiginfo(const siginfo_t* info, const char* message)
{
	psignal(info->si_signo, message);
}
