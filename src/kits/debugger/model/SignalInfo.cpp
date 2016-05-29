/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "SignalInfo.h"

#include <string.h>


SignalInfo::SignalInfo()
	:
	fSignal(0),
	fDeadly(false)
{
	memset(&fHandler, 0, sizeof(fHandler));
}


SignalInfo::SignalInfo(const SignalInfo& other)
	:
	fSignal(other.fSignal),
	fDeadly(other.fDeadly)
{
	memcpy(&fHandler, &other.fHandler, sizeof(fHandler));
}


SignalInfo::SignalInfo(int signal, const struct sigaction& handler,
	bool deadly)
	:
	fSignal(signal),
	fDeadly(deadly)
{
	memcpy(&fHandler, &handler, sizeof(fHandler));
}


void
SignalInfo::SetTo(int signal, const struct sigaction& handler, bool deadly)
{
	fSignal = signal;
	fDeadly = deadly;

	memcpy(&fHandler, &handler, sizeof(fHandler));
}
