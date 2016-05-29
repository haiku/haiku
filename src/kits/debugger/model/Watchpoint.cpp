/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "Watchpoint.h"


Watchpoint::Watchpoint(target_addr_t address, uint32 type, int32 length)
	:
	fAddress(address),
	fType(type),
	fLength(length),
	fInstalled(false),
	fEnabled(false)
{
}


Watchpoint::~Watchpoint()
{
}


void
Watchpoint::SetInstalled(bool installed)
{
	fInstalled = installed;
}


void
Watchpoint::SetEnabled(bool enabled)
{
	fEnabled = enabled;
}


bool
Watchpoint::Contains(target_addr_t address) const
{
	return address >= fAddress && address <= (fAddress + fLength);
}


int
Watchpoint::CompareWatchpoints(const Watchpoint* a, const Watchpoint* b)
{
	if (a->Address() < b->Address())
		return -1;
	return a->Address() == b->Address() ? 0 : 1;
}


int
Watchpoint::CompareAddressWatchpoint(const target_addr_t* address,
	const Watchpoint* watchpoint)
{
	if (*address < watchpoint->Address())
		return -1;
	return *address == watchpoint->Address() ? 0 : 1;
}



