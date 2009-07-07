/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "UserBreakpoint.h"

#include "Function.h"


// #pragma mark - UserBreakpointInstance


UserBreakpointInstance::UserBreakpointInstance(UserBreakpoint* userBreakpoint,
	target_addr_t address)
	:
	fAddress(address),
	fUserBreakpoint(userBreakpoint),
	fBreakpoint(NULL)
{
}


void
UserBreakpointInstance::SetBreakpoint(Breakpoint* breakpoint)
{
	fBreakpoint = breakpoint;
}


// #pragma mark - UserBreakpoint


UserBreakpoint::UserBreakpoint(Function* function)
	:
	fFunction(function),
	fValid(false),
	fEnabled(false)
{
	fFunction->AcquireReference();
}


UserBreakpoint::~UserBreakpoint()
{
	for (int32 i = 0; UserBreakpointInstance* instance = fInstances.ItemAt(i);
			i++) {
		delete instance;
	}

	fFunction->ReleaseReference();
}


int32
UserBreakpoint::CountInstances() const
{
	return fInstances.CountItems();
}


UserBreakpointInstance*
UserBreakpoint::InstanceAt(int32 index) const
{
	return fInstances.ItemAt(index);
}


bool
UserBreakpoint::AddInstance(UserBreakpointInstance* instance)
{
	return fInstances.AddItem(instance);
}


void
UserBreakpoint::RemoveInstance(UserBreakpointInstance* instance)
{
	fInstances.RemoveItem(instance);
}


void
UserBreakpoint::SetValid(bool valid)
{
	fValid = valid;
}


void
UserBreakpoint::SetEnabled(bool enabled)
{
	fEnabled = enabled;
}
