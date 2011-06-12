/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <signal_defs.h>


int
__signal_get_sigrtmin()
{
	return SIGNAL_REALTIME_MIN;
}


int
__signal_get_sigrtmax()
{
	return SIGNAL_REALTIME_MAX;
}
