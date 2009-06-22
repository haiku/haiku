/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Breakpoint.h"


// #pragma mark - BreakpointClient


BreakpointClient::~BreakpointClient()
{
}


// #pragma mark - Breakpoint


Breakpoint::Breakpoint(Image* image, target_addr_t address)
	:
	fAddress(address),
	fImage(image),
	fUserState(USER_BREAKPOINT_NONE),
	fInstalled(false)
{
}


Breakpoint::~Breakpoint()
{
}


void
Breakpoint::SetUserState(user_breakpoint_state state)
{
	fUserState = state;
}


void
Breakpoint::SetInstalled(bool installed)
{
	fInstalled = installed;
}


bool
Breakpoint::ShouldBeInstalled() const
{
	return fUserState == USER_BREAKPOINT_ENABLED || !fClients.IsEmpty();
}


bool
Breakpoint::IsUnused() const
{
	return fUserState == USER_BREAKPOINT_NONE && fClients.IsEmpty();
}


bool
Breakpoint::AddClient(BreakpointClient* client)
{
	return fClients.AddItem(client);
}


void
Breakpoint::RemoveClient(BreakpointClient* client)
{
	fClients.RemoveItem(client);
}


/*static*/ int
Breakpoint::CompareBreakpoints(const Breakpoint* a, const Breakpoint* b)
{
	if (a->Address() < b->Address())
		return -1;
	return a->Address() == b->Address() ? 0 : 1;
}


/*static*/ int
Breakpoint::CompareAddressBreakpoint(const target_addr_t* address,
	const Breakpoint* breakpoint)
{
	if (*address < breakpoint->Address())
		return -1;
	return *address == breakpoint->Address() ? 0 : 1;
}
