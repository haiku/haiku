/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TeamDebugModel.h"

#include <new>

#include <AutoLocker.h>

#include "Breakpoint.h"
#include "Function.h"
#include "UserBreakpoint.h"


// #pragma mark - BreakpointByAddressPredicate


struct TeamDebugModel::BreakpointByAddressPredicate
	: UnaryPredicate<Breakpoint> {
	BreakpointByAddressPredicate(target_addr_t address)
		:
		fAddress(address)
	{
	}

	virtual int operator()(const Breakpoint* breakpoint) const
	{
		return -Breakpoint::CompareAddressBreakpoint(&fAddress, breakpoint);
	}

private:
	target_addr_t	fAddress;
};



// #pragma mark - TeamDebugModel


TeamDebugModel::TeamDebugModel(Team* team, TeamMemory* teamMemory,
	Architecture* architecture)
	:
	fTeam(team),
	fTeamMemory(teamMemory),
	fArchitecture(architecture)
{
}


TeamDebugModel::~TeamDebugModel()
{
	for (int32 i = 0; Breakpoint* breakpoint = fBreakpoints.ItemAt(i); i++)
		breakpoint->RemoveReference();
}


status_t
TeamDebugModel::Init()
{
	return B_OK;
}


bool
TeamDebugModel::AddBreakpoint(Breakpoint* breakpoint)
{
	if (fBreakpoints.BinaryInsert(breakpoint, &Breakpoint::CompareBreakpoints))
		return true;

	breakpoint->RemoveReference();
	return false;
}


void
TeamDebugModel::RemoveBreakpoint(Breakpoint* breakpoint)
{
	int32 index = fBreakpoints.BinarySearchIndex(*breakpoint,
		&Breakpoint::CompareBreakpoints);
	if (index < 0)
		return;

	fBreakpoints.RemoveItemAt(index);
	breakpoint->RemoveReference();
}


int32
TeamDebugModel::CountBreakpoints() const
{
	return fBreakpoints.CountItems();
}


Breakpoint*
TeamDebugModel::BreakpointAt(int32 index) const
{
	return fBreakpoints.ItemAt(index);
}


Breakpoint*
TeamDebugModel::BreakpointAtAddress(target_addr_t address) const
{
	return fBreakpoints.BinarySearchByKey(address,
		&Breakpoint::CompareAddressBreakpoint);
}


//void
//TeamDebugModel::GetBreakpointsInAddressRange(TargetAddressRange range,
//	BObjectList<Breakpoint>& breakpoints) const
//{
//	int32 index = fBreakpoints.FindBinaryInsertionIndex(
//		BreakpointByAddressPredicate(range.Start()));
//	for (; Breakpoint* breakpoint = fBreakpoints.ItemAt(index); index++) {
//		if (breakpoint->Address() > range.End())
//			break;
//		breakpoints.AddItem(breakpoint);
//	}
//}


void
TeamDebugModel::GetBreakpointsForSourceCode(SourceCode* sourceCode,
	BObjectList<UserBreakpoint>& breakpoints) const
{
	// TODO: This can probably be optimized. Maybe by registering the user
	// breakpoints with the team debug model and sorting them by source code.
	for (int32 i = 0; Breakpoint* breakpoint = fBreakpoints.ItemAt(i); i++) {
		UserBreakpointInstance* userBreakpointInstance
			= breakpoint->FirstUserBreakpoint();
		if (userBreakpointInstance == NULL)
			continue;

		UserBreakpoint* userBreakpoint
			= userBreakpointInstance->GetUserBreakpoint();
		if (userBreakpoint->GetFunction()->GetSourceCode() == sourceCode)
			breakpoints.AddItem(userBreakpoint);
	}
}


void
TeamDebugModel::AddListener(Listener* listener)
{
	AutoLocker<TeamDebugModel> locker(this);
	fListeners.Add(listener);
}


void
TeamDebugModel::RemoveListener(Listener* listener)
{
	AutoLocker<TeamDebugModel> locker(this);
	fListeners.Remove(listener);
}


void
TeamDebugModel::NotifyUserBreakpointChanged(Breakpoint* breakpoint)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->UserBreakpointChanged(BreakpointEvent(
			TEAM_DEBUG_MODEL_EVENT_USER_BREAKPOINT_CHANGED, this, breakpoint));
	}
}


void
TeamDebugModel::_NotifyBreakpointAdded(Breakpoint* breakpoint)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->BreakpointAdded(BreakpointEvent(
			TEAM_DEBUG_MODEL_EVENT_BREAKPOINT_ADDED, this, breakpoint));
	}
}


void
TeamDebugModel::_NotifyBreakpointRemoved(Breakpoint* breakpoint)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->BreakpointRemoved(BreakpointEvent(
			TEAM_DEBUG_MODEL_EVENT_BREAKPOINT_REMOVED, this, breakpoint));
	}
}


// #pragma mark - Event


TeamDebugModel::Event::Event(uint32 type, TeamDebugModel* model)
	:
	fEventType(type),
	fModel(model)
{
}


// #pragma mark - ThreadEvent


TeamDebugModel::BreakpointEvent::BreakpointEvent(uint32 type,
	TeamDebugModel* model, Breakpoint* breakpoint)
	:
	Event(type, model),
	fBreakpoint(breakpoint)
{
}


// #pragma mark - Listener


TeamDebugModel::Listener::~Listener()
{
}


void
TeamDebugModel::Listener::BreakpointAdded(
	const TeamDebugModel::BreakpointEvent& event)
{
}


void
TeamDebugModel::Listener::BreakpointRemoved(
	const TeamDebugModel::BreakpointEvent& event)
{
}


void
TeamDebugModel::Listener::UserBreakpointChanged(
	const TeamDebugModel::BreakpointEvent& event)
{
}
