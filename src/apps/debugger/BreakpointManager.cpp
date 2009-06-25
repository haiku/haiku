/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "BreakpointManager.h"

#include <stdio.h>

#include <new>

#include <AutoLocker.h>

#include "DebuggerInterface.h"
#include "TeamDebugModel.h"


BreakpointManager::BreakpointManager(TeamDebugModel* debugModel,
	DebuggerInterface* debuggerInterface)
	:
	fLock("breakpoint manager"),
	fDebugModel(debugModel),
	fDebuggerInterface(debuggerInterface)
{
}


BreakpointManager::~BreakpointManager()
{
}


status_t
BreakpointManager::Init()
{
	return fLock.InitCheck();
}


status_t
BreakpointManager::InstallUserBreakpoint(target_addr_t address,
	bool enabled)
{
	user_breakpoint_state state = enabled
		? USER_BREAKPOINT_ENABLED : USER_BREAKPOINT_DISABLED;

	AutoLocker<TeamDebugModel> modelLocker(fDebugModel);

	// If there already is a breakpoint, it might already have the requested
	// state.
	Breakpoint* breakpoint = fDebugModel->BreakpointAtAddress(address);
	if (breakpoint != NULL && breakpoint->UserState() == state)
		return B_OK;

	// create a breakpoint, if it doesn't exist yet
	if (breakpoint == NULL) {
		Image* image = fDebugModel->GetTeam()->ImageByAddress(address);
		if (image == NULL)
			return B_BAD_ADDRESS;

		breakpoint = new(std::nothrow) Breakpoint(image, address);
		if (breakpoint == NULL)
			return B_NO_MEMORY;

		if (!fDebugModel->AddBreakpoint(breakpoint))
			return B_NO_MEMORY;
	}

	user_breakpoint_state oldState = breakpoint->UserState();

	// set the breakpoint state
	breakpoint->SetUserState(state);
	fDebugModel->NotifyUserBreakpointChanged(breakpoint);

	AutoLocker<BLocker> installLocker(fLock);
		// We need to make the installation decision with both locks held, and
		// we keep this lock until we have the breakpoint installed/uninstalled.

	bool install = breakpoint->ShouldBeInstalled();
	if (breakpoint->IsInstalled() == install)
		return B_OK;

	// The breakpoint needs to be installed/uninstalled.
	Reference<Breakpoint> breakpointReference(breakpoint);
	modelLocker.Unlock();

	status_t error = install
		? fDebuggerInterface->InstallBreakpoint(address)
		: fDebuggerInterface->UninstallBreakpoint(address);

	// Mark the breakpoint installed/uninstalled, if everything went fine.
	if (error == B_OK) {
		breakpoint->SetInstalled(install);
		return B_OK;
	}

	// revert on error
	installLocker. Unlock();
	modelLocker.Lock();

	breakpoint->SetUserState(oldState);
	fDebugModel->NotifyUserBreakpointChanged(breakpoint);

	if (breakpoint->IsUnused())
		fDebugModel->RemoveBreakpoint(breakpoint);

	return error;
}


void
BreakpointManager::UninstallUserBreakpoint(target_addr_t address)
{
	AutoLocker<TeamDebugModel> modelLocker(fDebugModel);

	Breakpoint* breakpoint = fDebugModel->BreakpointAtAddress(address);
	if (breakpoint == NULL || breakpoint->UserState() == USER_BREAKPOINT_NONE)
		return;

	// set the breakpoint state
	breakpoint->SetUserState(USER_BREAKPOINT_NONE);
	fDebugModel->NotifyUserBreakpointChanged(breakpoint);

	AutoLocker<BLocker> installLocker(fLock);
		// We need to make the uninstallation decision with both locks held, and
		// we keep this lock until we have the breakpoint uninstalled.

	// check whether the breakpoint needs to be uninstalled
	bool uninstall = !breakpoint->ShouldBeInstalled()
		&& breakpoint->IsInstalled();

	// if unused remove it
	Reference<Breakpoint> breakpointReference(breakpoint);
	if (breakpoint->IsUnused())
		fDebugModel->RemoveBreakpoint(breakpoint);

	modelLocker.Unlock();

	if (uninstall) {
		fDebuggerInterface->UninstallBreakpoint(address);
		breakpoint->SetInstalled(false);
	}
}


status_t
BreakpointManager::InstallTemporaryBreakpoint(target_addr_t address,
	BreakpointClient* client)
{
	AutoLocker<TeamDebugModel> modelLocker(fDebugModel);

	// create a breakpoint, if it doesn't exist yet
	Breakpoint* breakpoint = fDebugModel->BreakpointAtAddress(address);
	if (breakpoint == NULL) {
		Image* image = fDebugModel->GetTeam()->ImageByAddress(address);
		if (image == NULL)
			return B_BAD_ADDRESS;

		breakpoint = new(std::nothrow) Breakpoint(image, address);
		if (breakpoint == NULL)
			return B_NO_MEMORY;

		if (!fDebugModel->AddBreakpoint(breakpoint))
			return B_NO_MEMORY;
	}

	Reference<Breakpoint> breakpointReference(breakpoint);

	// add the client
	status_t error;
	if (breakpoint->AddClient(client)) {
		AutoLocker<BLocker> installLocker(fLock);
			// We need to make the installation decision with both locks held,
			// and we keep this lock until we have the breakpoint installed.

		if (breakpoint->IsInstalled())
			return B_OK;

		// install
		modelLocker.Unlock();

		error = fDebuggerInterface->InstallBreakpoint(address);
		if (error == B_OK) {
			breakpoint->SetInstalled(true);
			return B_OK;
		}

		installLocker.Unlock();
		modelLocker.Lock();

		breakpoint->RemoveClient(client);
	} else
		error = B_NO_MEMORY;

	// clean up on error
	if (breakpoint->IsUnused())
		fDebugModel->RemoveBreakpoint(breakpoint);

	return error;
}


void
BreakpointManager::UninstallTemporaryBreakpoint(target_addr_t address,
	BreakpointClient* client)
{
	AutoLocker<TeamDebugModel> modelLocker(fDebugModel);

	Breakpoint* breakpoint = fDebugModel->BreakpointAtAddress(address);
	if (breakpoint == NULL)
		return;

	// remove the client
	breakpoint->RemoveClient(client);

	AutoLocker<BLocker> installLocker(fLock);
		// We need to make the uninstallation decision with both locks held, and
		// we keep this lock until we have the breakpoint uninstalled.

	// check whether the breakpoint needs to be uninstalled
	bool uninstall = !breakpoint->ShouldBeInstalled()
		&& breakpoint->IsInstalled();

	// if unused remove it
	Reference<Breakpoint> breakpointReference(breakpoint);
	if (breakpoint->IsUnused())
		fDebugModel->RemoveBreakpoint(breakpoint);

	modelLocker.Unlock();

	if (uninstall) {
		fDebuggerInterface->UninstallBreakpoint(address);
		breakpoint->SetInstalled(false);
	}
}
