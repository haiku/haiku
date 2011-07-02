/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "BreakpointManager.h"

#include <stdio.h>

#include <new>

#include <AutoLocker.h>

#include "DebuggerInterface.h"
#include "Function.h"
#include "SpecificImageDebugInfo.h"
#include "Statement.h"
#include "Team.h"
#include "Tracing.h"


BreakpointManager::BreakpointManager(Team* team,
	DebuggerInterface* debuggerInterface)
	:
	fLock("breakpoint manager"),
	fTeam(team),
	fDebuggerInterface(debuggerInterface)
{
	fDebuggerInterface->AcquireReference();
}


BreakpointManager::~BreakpointManager()
{
	fDebuggerInterface->ReleaseReference();
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
	TRACE_CONTROL("BreakpointManager::InstallUserBreakpoint(%p, %d)\n",
		userBreakpoint, enabled);

	AutoLocker<BLocker> installLocker(fLock);
	AutoLocker<Team> teamLocker(fTeam);

	bool oldEnabled = userBreakpoint->IsEnabled();
	if (userBreakpoint->IsValid() && enabled == oldEnabled) {
		TRACE_CONTROL("  user breakpoint already valid and with same enabled "
			"state\n");
		return B_OK;
	}

	// get/create the breakpoints for all instances
	TRACE_CONTROL("  creating breakpoints for breakpoint instances\n");

	status_t error = B_OK;
	for (int32 i = 0;
		UserBreakpointInstance* instance = userBreakpoint->InstanceAt(i); i++) {

		TRACE_CONTROL("    breakpoint instance %p\n", instance);

		if (instance->GetBreakpoint() != NULL) {
			TRACE_CONTROL("    -> already has breakpoint\n");
			continue;
		}

		target_addr_t address = instance->Address();
		Breakpoint* breakpoint = fTeam->BreakpointAtAddress(address);
		if (breakpoint == NULL) {
			TRACE_CONTROL("    -> no breakpoint at that address yet\n");

			Image* image = fTeam->ImageByAddress(address);
			if (image == NULL) {
				TRACE_CONTROL("    -> no image at that address\n");
				error = B_BAD_ADDRESS;
				break;
			}

			breakpoint = new(std::nothrow) Breakpoint(image, address);
			if (breakpoint == NULL || !fTeam->AddBreakpoint(breakpoint)) {
				delete breakpoint;
				error = B_NO_MEMORY;
				break;
			}
		}

		TRACE_CONTROL("    -> adding instance to breakpoint %p\n", breakpoint);

		breakpoint->AddUserBreakpoint(instance);
		instance->SetBreakpoint(breakpoint);
	}

	// If everything looks good so far mark the user breakpoint according to
	// its new state.
	if (error == B_OK)
		userBreakpoint->SetEnabled(enabled);

	// notify user breakpoint listeners
	if (error == B_OK)
		fTeam->NotifyUserBreakpointChanged(userBreakpoint);

	teamLocker.Unlock();

	// install/uninstall the breakpoints as needed
	TRACE_CONTROL("  updating breakpoints\n");

	if (error == B_OK) {
		for (int32 i = 0;
			UserBreakpointInstance* instance = userBreakpoint->InstanceAt(i);
			i++) {
			TRACE_CONTROL("    breakpoint instance %p\n", instance);

			error = _UpdateBreakpointInstallation(instance->GetBreakpoint());
			if (error != B_OK)
				break;
		}
	}

	if (error == B_OK) {
		TRACE_CONTROL("  success, marking user breakpoint valid\n");

		// everything went fine -- mark the user breakpoint valid
		if (!userBreakpoint->IsValid()) {
			teamLocker.Lock();
			userBreakpoint->SetValid(true);
			userBreakpoint->AcquireReference();
			fTeam->AddUserBreakpoint(userBreakpoint);
			fTeam->NotifyUserBreakpointChanged(userBreakpoint);
				// notify again -- the breakpoint hadn't been added before
			teamLocker.Unlock();
		}
	} else {
		// something went wrong -- revert the situation
		TRACE_CONTROL("  error, reverting\n");

		teamLocker.Lock();
		userBreakpoint->SetEnabled(oldEnabled);
		teamLocker.Unlock();

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

				teamLocker.Lock();

				if (breakpoint->IsUnused())
					fTeam->RemoveBreakpoint(breakpoint);
				teamLocker.Unlock();
			}

			teamLocker.Lock();
			fTeam->NotifyUserBreakpointChanged(userBreakpoint);
			teamLocker.Unlock();
		}
	}

	installLocker.Unlock();

	return error;
}


void
BreakpointManager::UninstallUserBreakpoint(UserBreakpoint* userBreakpoint)
{
	AutoLocker<BLocker> installLocker(fLock);
	AutoLocker<Team> teamLocker(fTeam);

	if (!userBreakpoint->IsValid())
		return;

	fTeam->RemoveUserBreakpoint(userBreakpoint);

	userBreakpoint->SetValid(false);
	userBreakpoint->SetEnabled(false);

	teamLocker.Unlock();

	// uninstall the breakpoints as needed
	for (int32 i = 0;
		UserBreakpointInstance* instance = userBreakpoint->InstanceAt(i); i++) {
		if (Breakpoint* breakpoint = instance->GetBreakpoint())
			_UpdateBreakpointInstallation(breakpoint);
	}

	teamLocker.Lock();

	// detach the breakpoints from the user breakpoint instances
	for (int32 i = 0;
		UserBreakpointInstance* instance = userBreakpoint->InstanceAt(i); i++) {
		if (Breakpoint* breakpoint = instance->GetBreakpoint()) {
			instance->SetBreakpoint(NULL);
			breakpoint->RemoveUserBreakpoint(instance);

			if (breakpoint->IsUnused())
				fTeam->RemoveBreakpoint(breakpoint);
		}
	}

	fTeam->NotifyUserBreakpointChanged(userBreakpoint);

	teamLocker.Unlock();
	installLocker.Unlock();

	// release the reference from InstallUserBreakpoint()
	userBreakpoint->ReleaseReference();
}


status_t
BreakpointManager::InstallTemporaryBreakpoint(target_addr_t address,
	BreakpointClient* client)
{
	AutoLocker<BLocker> installLocker(fLock);
	AutoLocker<Team> teamLocker(fTeam);

	// create a breakpoint, if it doesn't exist yet
	Breakpoint* breakpoint = fTeam->BreakpointAtAddress(address);
	if (breakpoint == NULL) {
		Image* image = fTeam->ImageByAddress(address);
		if (image == NULL)
			return B_BAD_ADDRESS;

		breakpoint = new(std::nothrow) Breakpoint(image, address);
		if (breakpoint == NULL)
			return B_NO_MEMORY;

		if (!fTeam->AddBreakpoint(breakpoint))
			return B_NO_MEMORY;
	}

	BReference<Breakpoint> breakpointReference(breakpoint);

	// add the client
	status_t error;
	if (breakpoint->AddClient(client)) {
		if (breakpoint->IsInstalled())
			return B_OK;

		// install
		teamLocker.Unlock();

		error = fDebuggerInterface->InstallBreakpoint(address);
		if (error == B_OK) {
			breakpoint->SetInstalled(true);
			return B_OK;
		}

		teamLocker.Lock();

		breakpoint->RemoveClient(client);
	} else
		error = B_NO_MEMORY;

	// clean up on error
	if (breakpoint->IsUnused())
		fTeam->RemoveBreakpoint(breakpoint);

	return error;
}


void
BreakpointManager::UninstallTemporaryBreakpoint(target_addr_t address,
	BreakpointClient* client)
{
	AutoLocker<BLocker> installLocker(fLock);
	AutoLocker<Team> teamLocker(fTeam);

	Breakpoint* breakpoint = fTeam->BreakpointAtAddress(address);
	if (breakpoint == NULL)
		return;

	// remove the client
	breakpoint->RemoveClient(client);

	// check whether the breakpoint needs to be uninstalled
	bool uninstall = !breakpoint->ShouldBeInstalled()
		&& breakpoint->IsInstalled();

	// if unused remove it
	BReference<Breakpoint> breakpointReference(breakpoint);
	if (breakpoint->IsUnused())
		fTeam->RemoveBreakpoint(breakpoint);

	teamLocker.Unlock();

	if (uninstall) {
		fDebuggerInterface->UninstallBreakpoint(address);
		breakpoint->SetInstalled(false);
	}
}


void
BreakpointManager::UpdateImageBreakpoints(Image* image)
{
	_UpdateImageBreakpoints(image, false);
}


void
BreakpointManager::RemoveImageBreakpoints(Image* image)
{
	_UpdateImageBreakpoints(image, true);
}


void
BreakpointManager::_UpdateImageBreakpoints(Image* image, bool removeOnly)
{
	AutoLocker<BLocker> installLocker(fLock);
	AutoLocker<Team> teamLocker(fTeam);

	// remove obsolete user breakpoint instances
	BObjectList<Breakpoint> breakpointsToUpdate;
	for (UserBreakpointList::ConstIterator it
			= fTeam->UserBreakpoints().GetIterator();
		UserBreakpoint* userBreakpoint = it.Next();) {
		int32 instanceCount = userBreakpoint->CountInstances();
		for (int32 i = instanceCount - 1; i >= 0; i--) {
			UserBreakpointInstance* instance = userBreakpoint->InstanceAt(i);
			Breakpoint* breakpoint = instance->GetBreakpoint();
			if (breakpoint == NULL || breakpoint->GetImage() != image)
				continue;

			userBreakpoint->RemoveInstanceAt(i);
			breakpoint->RemoveUserBreakpoint(instance);

			if (!breakpointsToUpdate.AddItem(breakpoint)) {
				_UpdateBreakpointInstallation(breakpoint);
				if (breakpoint->IsUnused())
					fTeam->RemoveBreakpoint(breakpoint);
			}

			delete instance;
		}
	}

	// update breakpoints
	teamLocker.Unlock();
	for (int32 i = 0; Breakpoint* breakpoint = breakpointsToUpdate.ItemAt(i);
			i++) {
		_UpdateBreakpointInstallation(breakpoint);
	}

	teamLocker.Lock();
	for (int32 i = 0; Breakpoint* breakpoint = breakpointsToUpdate.ItemAt(i);
			i++) {
		if (breakpoint->IsUnused())
			fTeam->RemoveBreakpoint(breakpoint);
	}

	// add breakpoint instances for function instances in the image (if we have
	// an image debug info)
	BObjectList<UserBreakpointInstance> newInstances;
	ImageDebugInfo* imageDebugInfo = image->GetImageDebugInfo();
	if (imageDebugInfo == NULL)
		return;

	for (UserBreakpointList::ConstIterator it
			= fTeam->UserBreakpoints().GetIterator();
		UserBreakpoint* userBreakpoint = it.Next();) {
		// get the function
		Function* function = fTeam->FunctionByID(
			userBreakpoint->Location().GetFunctionID());
		if (function == NULL)
			continue;

		const SourceLocation& sourceLocation
			= userBreakpoint->Location().GetSourceLocation();
		target_addr_t relativeAddress
			= userBreakpoint->Location().RelativeAddress();

		// iterate through the function instances
		for (FunctionInstanceList::ConstIterator it
				= function->Instances().GetIterator();
			FunctionInstance* functionInstance = it.Next();) {
			if (functionInstance->GetImageDebugInfo() != imageDebugInfo)
				continue;

			// get the breakpoint address for the instance
			target_addr_t instanceAddress = 0;
			if (functionInstance->SourceFile() != NULL) {
				// We have a source file, so get the address for the source
				// location.
				Statement* statement = NULL;
				FunctionDebugInfo* functionDebugInfo
					= functionInstance->GetFunctionDebugInfo();
				functionDebugInfo->GetSpecificImageDebugInfo()
					->GetStatementAtSourceLocation(functionDebugInfo,
						sourceLocation, statement);
				if (statement != NULL) {
					instanceAddress = statement->CoveringAddressRange().Start();
						// TODO: What about BreakpointAllowed()?
					statement->ReleaseReference();
					// TODO: Make sure we do hit the function in question!
				}
			}

			if (instanceAddress == 0) {
				// No source file (or we failed getting the statement), so try
				// to use the same relative address.
				if (relativeAddress > functionInstance->Size())
					continue;
				instanceAddress = functionInstance->Address() + relativeAddress;
					// TODO: Make sure it does at least hit an instruction!
			}

			// create the user breakpoint instance
			UserBreakpointInstance* instance = new(std::nothrow)
				UserBreakpointInstance(userBreakpoint, instanceAddress);
			if (instance == NULL || !newInstances.AddItem(instance)) {
				delete instance;
				continue;
			}

			if (!userBreakpoint->AddInstance(instance)) {
				newInstances.RemoveItemAt(newInstances.CountItems() - 1);
				delete instance;
			}

			// get/create the breakpoint for the address
			target_addr_t address = instance->Address();
			Breakpoint* breakpoint = fTeam->BreakpointAtAddress(address);
			if (breakpoint == NULL) {
				breakpoint = new(std::nothrow) Breakpoint(image, address);
				if (breakpoint == NULL || !fTeam->AddBreakpoint(breakpoint)) {
					delete breakpoint;
					break;
				}
			}

			breakpoint->AddUserBreakpoint(instance);
			instance->SetBreakpoint(breakpoint);
		}
	}

	// install the breakpoints for the new user breakpoint instances
	teamLocker.Unlock();
	for (int32 i = 0; UserBreakpointInstance* instance = newInstances.ItemAt(i);
			i++) {
		Breakpoint* breakpoint = instance->GetBreakpoint();
		if (breakpoint == NULL
			|| _UpdateBreakpointInstallation(breakpoint) != B_OK) {
			// something went wrong -- remove the instance
			teamLocker.Lock();

			instance->GetUserBreakpoint()->RemoveInstance(instance);
			if (breakpoint != NULL) {
				breakpoint->AddUserBreakpoint(instance);
				if (breakpoint->IsUnused())
					fTeam->RemoveBreakpoint(breakpoint);
			}

			teamLocker.Unlock();
		}
	}
}


status_t
BreakpointManager::_UpdateBreakpointInstallation(Breakpoint* breakpoint)
{
	bool shouldBeInstalled = breakpoint->ShouldBeInstalled();

	TRACE_CONTROL("BreakpointManager::_UpdateBreakpointInstallation(%p): "
		"should be installed: %d, is installed: %d\n", breakpoint,
		shouldBeInstalled, breakpoint->IsInstalled());

	if (shouldBeInstalled == breakpoint->IsInstalled())
		return B_OK;

	if (shouldBeInstalled) {
		// install
		status_t error = fDebuggerInterface->InstallBreakpoint(
			breakpoint->Address());
		if (error != B_OK)
			return error;

		TRACE_CONTROL("BREAKPOINT at %#llx installed: %s\n",
			breakpoint->Address(), strerror(error));

		breakpoint->SetInstalled(true);
	} else {
		// uninstall
		fDebuggerInterface->UninstallBreakpoint(breakpoint->Address());

		TRACE_CONTROL("BREAKPOINT at %#llx uninstalled\n",
			breakpoint->Address());

		breakpoint->SetInstalled(false);
	}

	return B_OK;
}
