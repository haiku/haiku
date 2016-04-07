/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TargetHostInterface.h"

#include "TeamDebugger.h"


TargetHostInterface::TargetHostInterface()
	:
	BReferenceable(),
	fTeamDebuggers(20, false)
{
}


TargetHostInterface::~TargetHostInterface()
{
}


void
TargetHostInterface::SetName(const BString& name)
{
	fName = name;
}


int32
TargetHostInterface::CountTeamDebuggers() const
{
	return fTeamDebuggers.CountItems();
}


TeamDebugger*
TargetHostInterface::TeamDebuggerAt(int32 index) const
{
	return fTeamDebuggers.ItemAt(index);
}


TeamDebugger*
TargetHostInterface::FindTeamDebugger(team_id team) const
{
	return fTeamDebuggers.BinarySearchByKey(team, &_FindDebuggerByKey);
}


status_t
TargetHostInterface::AddTeamDebugger(TeamDebugger* debugger)
{
	if (!fTeamDebuggers.BinaryInsert(debugger, &_CompareDebuggers))
		return B_NO_MEMORY;

	return B_OK;
}


void
TargetHostInterface::RemoveTeamDebugger(TeamDebugger* debugger)
{
	int32 index = fTeamDebuggers.BinarySearchIndexByKey(debugger->TeamID(),
		&_FindDebuggerByKey);
	if (index >= 0)
		fTeamDebuggers.RemoveItemAt(index);
}


/*static*/ int
TargetHostInterface::_CompareDebuggers(const TeamDebugger* a,
	const TeamDebugger* b)
{
	return a->TeamID() < b->TeamID() ? -1 : 1;
}


/*static*/ int
TargetHostInterface::_FindDebuggerByKey(const team_id* team,
	const TeamDebugger* debugger)
{
	if (*team < debugger->TeamID())
		return -1;
	else if (*team > debugger->TeamID())
		return 1;
	return 0;
}
