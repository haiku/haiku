/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BREAKPOINT_MANAGER_H
#define BREAKPOINT_MANAGER_H

#include <Locker.h>

#include "Breakpoint.h"


class DebuggerInterface;
class Team;


class BreakpointManager {
public:
								BreakpointManager(Team* team,
									DebuggerInterface* debuggerInterface);
								~BreakpointManager();

			status_t			Init();

			status_t			InstallUserBreakpoint(
									UserBreakpoint* userBreakpoint,
									bool enabled);
			void				UninstallUserBreakpoint(
									UserBreakpoint* userBreakpoint);

			status_t			InstallTemporaryBreakpoint(
									target_addr_t address,
									BreakpointClient* client);
			void				UninstallTemporaryBreakpoint(
									target_addr_t address,
									BreakpointClient* client);

			void				UpdateImageBreakpoints(Image* image);
			void				RemoveImageBreakpoints(Image* image);

private:
			void				_UpdateImageBreakpoints(Image* image,
									bool removeOnly);
			status_t			_UpdateBreakpointInstallation(
									Breakpoint* breakpoint);
										// fLock must be held

private:
			BLocker				fLock;	// used to synchronize un-/installing
			Team*				fTeam;
			DebuggerInterface*	fDebuggerInterface;
};


#endif	// BREAKPOINT_MANAGER_H
