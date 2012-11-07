/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef WATCHPOINT_MANAGER_H
#define WATCHPOINT_MANAGER_H

#include <Locker.h>

#include "Watchpoint.h"


class DebuggerInterface;
class Team;


class WatchpointManager {
public:
								WatchpointManager(Team* team,
									DebuggerInterface* debuggerInterface);
								~WatchpointManager();

			status_t			Init();

			status_t			InstallWatchpoint(Watchpoint* watchpoint,
									bool enabled);
			void				UninstallWatchpoint(Watchpoint* watchpoint);

private:
			BLocker				fLock;	// used to synchronize un-/installing
			Team*				fTeam;
			DebuggerInterface*	fDebuggerInterface;
};


#endif	// WATCHPOINT_MANAGER_H
