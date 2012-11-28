/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "WatchpointManager.h"

#include <stdio.h>

#include <new>

#include <AutoLocker.h>

#include "DebuggerInterface.h"
#include "Team.h"
#include "Tracing.h"


WatchpointManager::WatchpointManager(Team* team,
	DebuggerInterface* debuggerInterface)
	:
	fLock("watchpoint manager"),
	fTeam(team),
	fDebuggerInterface(debuggerInterface)
{
	fDebuggerInterface->AcquireReference();
}


WatchpointManager::~WatchpointManager()
{
	fDebuggerInterface->ReleaseReference();
}


status_t
WatchpointManager::Init()
{
	return fLock.InitCheck();
}


status_t
WatchpointManager::InstallWatchpoint(Watchpoint* watchpoint,
	bool enabled)
{
	status_t error = B_OK;
	TRACE_CONTROL("WatchpointManager::InstallUserWatchpoint(%p, %d)\n",
		watchpoint, enabled);

	AutoLocker<BLocker> installLocker(fLock);
	AutoLocker<Team> teamLocker(fTeam);

	bool oldEnabled = watchpoint->IsEnabled();
	if (enabled == oldEnabled) {
		TRACE_CONTROL("  watchpoint already valid and with same enabled "
			"state\n");
		return B_OK;
	}

	watchpoint->SetEnabled(enabled);

	if (watchpoint->ShouldBeInstalled()) {
		error = fDebuggerInterface->InstallWatchpoint(watchpoint->Address(),
			watchpoint->Type(), watchpoint->Length());

		if (error == B_OK)
			watchpoint->SetInstalled(true);
	} else {
		error = fDebuggerInterface->UninstallWatchpoint(watchpoint->Address());

		if (error == B_OK)
			watchpoint->SetInstalled(false);
	}

	if (error == B_OK) {
		if (fTeam->WatchpointAtAddress(watchpoint->Address()) == NULL)
			fTeam->AddWatchpoint(watchpoint);
		fTeam->NotifyWatchpointChanged(watchpoint);
	}

	return error;
}


void
WatchpointManager::UninstallWatchpoint(Watchpoint* watchpoint)
{
	AutoLocker<BLocker> installLocker(fLock);
	AutoLocker<Team> teamLocker(fTeam);

	fTeam->RemoveWatchpoint(watchpoint);

	if (!watchpoint->IsInstalled())
		return;

	status_t error = fDebuggerInterface->UninstallWatchpoint(
		watchpoint->Address());

	if (error == B_OK) {
		watchpoint->SetInstalled(false);
		fTeam->NotifyWatchpointChanged(watchpoint);
	}
}
