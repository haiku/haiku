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
BreakpointManager::InstallUserBreakpoint(UserBreakpoint* userBreakpoint,
	bool enabled)
{
printf("BreakpointManager::InstallUserBreakpoint(%p, %d)\n", userBreakpoint, enabled);
	AutoLocker<BLocker> installLocker(fLock);
	AutoLocker<TeamDebugModel> modelLocker(fDebugModel);

	bool oldEnabled = userBreakpoint->IsEnabled();
	if (userBreakpoint->IsValid() && enabled == oldEnabled)
{
printf("  user breakpoint already valid and with same enabled state\n");
		return B_OK;
}

	// get/create the breakpoints for all instances
printf("  creating breakpoints for breakpoint instances\n");
	status_t error = B_OK;
	for (int32 i = 0;
		UserBreakpointInstance* instance = userBreakpoint->InstanceAt(i); i++) {
printf("    breakpoint instance %p\n", instance);
		if (instance->GetBreakpoint() != NULL)
{
printf("    -> already has breakpoint\n");
			continue;
}

		target_addr_t address = instance->Address();
		Breakpoint* breakpoint = fDebugModel->BreakpointAtAddress(address);
		if (breakpoint == NULL) {
printf("    -> no breakpoint at that address yet\n");
			Image* image = fDebugModel->GetTeam()->ImageByAddress(address);
			if (image == NULL) {
printf("    -> no image at that address\n");
				error = B_BAD_ADDRESS;
				break;
			}

			breakpoint = new(std::nothrow) Breakpoint(image, address);
			if (breakpoint == NULL) {
				error = B_NO_MEMORY;
				break;
			}

			if (!fDebugModel->AddBreakpoint(breakpoint)) {
				error = B_NO_MEMORY;
				break;
			}
		}

printf("    -> adding instance to breakpoint %p\n", breakpoint);
		breakpoint->AddUserBreakpoint(instance);
		instance->SetBreakpoint(breakpoint);
	}

	// If everything looks good so far mark the user breakpoint according to
	// its new state.
	if (error == B_OK)
		userBreakpoint->SetEnabled(enabled);

	// notify user breakpoint listeners
	if (error == B_OK) {
		for (int32 i = 0;
			UserBreakpointInstance* instance = userBreakpoint->InstanceAt(i);
			i++) {
			fDebugModel->NotifyUserBreakpointChanged(instance->GetBreakpoint());
		}
	}

	modelLocker.Unlock();

	// install/uninstall the breakpoints as needed
printf("  updating breakpoints\n");
	if (error == B_OK) {
		for (int32 i = 0;
			UserBreakpointInstance* instance = userBreakpoint->InstanceAt(i);
			i++) {
printf("    breakpoint instance %p\n", instance);
			error = _UpdateBreakpointInstallation(instance->GetBreakpoint());
			if (error != B_OK)
				break;
		}
	}

	if (error == B_OK) {
printf("  success, marking user breakpoint valid\n");
		// everything went fine -- mark the user breakpoint valid
		if (!userBreakpoint->IsValid()) {
			modelLocker.Lock();
			userBreakpoint->SetValid(true);
			userBreakpoint->AcquireReference();
				// TODO: Put the user breakpoint some place?
			modelLocker.Unlock();
		}
	} else {
		// something went wrong -- revert the situation
printf("  error, reverting\n");
		modelLocker.Lock();
		userBreakpoint->SetEnabled(oldEnabled);
		modelLocker.Unlock();

		if (!oldEnabled || !userBreakpoint->IsValid()) {
			for (int32 i = 0;  UserBreakpointInstance* instance
					= userBreakpoint->InstanceAt(i);
				i++) {
				Breakpoint* breakpoint = instance->GetBreakpoint();
				if (breakpoint == NULL)
					continue;

				if (!userBreakpoint->IsValid()) {
					instance->SetBreakpoint(NULL);
					breakpoint->RemoveUserBreakpoint(instance);
				}

				_UpdateBreakpointInstallation(breakpoint);

				modelLocker.Lock();
				fDebugModel->NotifyUserBreakpointChanged(breakpoint);

				if (breakpoint->IsUnused())
					fDebugModel->RemoveBreakpoint(breakpoint);
				modelLocker.Unlock();
			}
		}
	}

	installLocker.Unlock();

	return error;
}


void
BreakpointManager::UninstallUserBreakpoint(UserBreakpoint* userBreakpoint)
{
	AutoLocker<BLocker> installLocker(fLock);
	AutoLocker<TeamDebugModel> modelLocker(fDebugModel);

	if (!userBreakpoint->IsValid())
		return;

	userBreakpoint->SetValid(false);
	userBreakpoint->SetEnabled(false);

	modelLocker.Unlock();

	// uninstall the breakpoints as needed
	for (int32 i = 0;
		UserBreakpointInstance* instance = userBreakpoint->InstanceAt(i); i++) {
		if (Breakpoint* breakpoint = instance->GetBreakpoint())
			_UpdateBreakpointInstallation(breakpoint);
	}

	modelLocker.Lock();

	// detach the breakpoints from the user breakpoint instances
	for (int32 i = 0;
		UserBreakpointInstance* instance = userBreakpoint->InstanceAt(i); i++) {
		if (Breakpoint* breakpoint = instance->GetBreakpoint()) {
			instance->SetBreakpoint(NULL);
			breakpoint->RemoveUserBreakpoint(instance);

			fDebugModel->NotifyUserBreakpointChanged(breakpoint);

			if (breakpoint->IsUnused())
				fDebugModel->RemoveBreakpoint(breakpoint);
		}
	}

	modelLocker.Unlock();
	installLocker.Unlock();

	// release the reference from InstallUserBreakpoint()
	userBreakpoint->ReleaseReference();
}


status_t
BreakpointManager::InstallTemporaryBreakpoint(target_addr_t address,
	BreakpointClient* client)
{
	AutoLocker<BLocker> installLocker(fLock);
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
		if (breakpoint->IsInstalled())
			return B_OK;

		// install
		modelLocker.Unlock();

		error = fDebuggerInterface->InstallBreakpoint(address);
		if (error == B_OK) {
			breakpoint->SetInstalled(true);
			return B_OK;
		}

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
	AutoLocker<BLocker> installLocker(fLock);
	AutoLocker<TeamDebugModel> modelLocker(fDebugModel);

	Breakpoint* breakpoint = fDebugModel->BreakpointAtAddress(address);
	if (breakpoint == NULL)
		return;

	// remove the client
	breakpoint->RemoveClient(client);

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
BreakpointManager::_UpdateBreakpointInstallation(Breakpoint* breakpoint)
{
	bool shouldBeInstalled = breakpoint->ShouldBeInstalled();
printf("BreakpointManager::_UpdateBreakpointInstallation(%p): should be installed: %d, is installed: %d\n", breakpoint, shouldBeInstalled, breakpoint->IsInstalled());
	if (shouldBeInstalled == breakpoint->IsInstalled())
		return B_OK;

	if (shouldBeInstalled) {
		// install
		status_t error = fDebuggerInterface->InstallBreakpoint(
			breakpoint->Address());
		if (error != B_OK)
			return error;
printf("BREAKPOINT at %#llx installed: %s\n", breakpoint->Address(), strerror(error));
		breakpoint->SetInstalled(true);
	} else {
		// uninstall
		fDebuggerInterface->UninstallBreakpoint(breakpoint->Address());
printf("BREAKPOINT at %#llx uninstalled\n", breakpoint->Address());
		breakpoint->SetInstalled(false);
	}

	return B_OK;
}
