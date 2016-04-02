/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TargetHost.h"

#include <AutoLocker.h>

#include "TeamInfo.h"


TargetHost::TargetHost(const BString& name)
	:
	BReferenceable(),
	fName(name),
	fLock(),
	fListeners(),
	fTeams()
{
}


TargetHost::~TargetHost()
{
	while (!fTeams.IsEmpty())
		delete fTeams.RemoveItemAt((int32)0);
}


void
TargetHost::AddListener(Listener* listener)
{
	AutoLocker<TargetHost> hostLocker(this);
	fListeners.Add(listener);
}


void
TargetHost::RemoveListener(Listener* listener)
{
	AutoLocker<TargetHost> hostLocker(this);
	fListeners.Remove(listener);
}


int32
TargetHost::CountTeams() const
{
	return fTeams.CountItems();
}


status_t
TargetHost::AddTeam(const team_info& info)
{
	TeamInfo* teamInfo = new (std::nothrow) TeamInfo(info.team, info);
	if (teamInfo == NULL)
		return B_NO_MEMORY;

	if (!fTeams.BinaryInsert(teamInfo, &_CompareTeams))
		return B_NO_MEMORY;

	_NotifyTeamAdded(teamInfo);
	return B_OK;
}


void
TargetHost::RemoveTeam(team_id team)
{
	int32 index = fTeams.BinarySearchIndexByKey(team,
		&_FindTeamByKey);
	if (index < 0)
		return;

	_NotifyTeamRemoved(team);
	TeamInfo* info = fTeams.RemoveItemAt(index);
	delete info;
}


void
TargetHost::UpdateTeam(const team_info& info)
{
	int32 index = fTeams.BinarySearchIndexByKey(info.team,
		&_FindTeamByKey);
	if (index < 0)
		return;

	TeamInfo* teamInfo = fTeams.ItemAt(index);
	teamInfo->SetTo(info.team, info);
	_NotifyTeamRenamed(teamInfo);
}


TeamInfo*
TargetHost::TeamInfoAt(int32 index) const
{
	return fTeams.ItemAt(index);
}


TeamInfo*
TargetHost::TeamInfoByID(team_id team) const
{
	return fTeams.BinarySearchByKey(team, &_FindTeamByKey);
}


/*static*/ int
TargetHost::_CompareTeams(const TeamInfo* a, const TeamInfo* b)
{
	return a->TeamID() < b->TeamID() ? -1 : 1;
}


/*static*/ int
TargetHost::_FindTeamByKey(const team_id* id, const TeamInfo* info)
{
	if (*id < info->TeamID())
		return -1;
	else if (*id > info->TeamID())
		return 1;
	return 0;
}


void
TargetHost::_NotifyTeamAdded(TeamInfo* info)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->TeamAdded(info);
	}
}


void
TargetHost::_NotifyTeamRemoved(team_id team)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->TeamRemoved(team);
	}
}


void
TargetHost::_NotifyTeamRenamed(TeamInfo* info)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->TeamRenamed(info);
	}
}


// #pragma mark - TargetHost::Listener


TargetHost::Listener::~Listener()
{
}


void
TargetHost::Listener::TeamAdded(TeamInfo* info)
{
}


void
TargetHost::Listener::TeamRemoved(team_id team)
{
}


void
TargetHost::Listener::TeamRenamed(TeamInfo* info)
{
}
