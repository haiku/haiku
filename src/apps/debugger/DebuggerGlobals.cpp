/*
 * Copyright 2009-2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "DebuggerGlobals.h"

#include "ImageDebugLoadingStateHandlerRoster.h"
#include "TargetHostInterface.h"
#include "TypeHandlerRoster.h"


status_t
debugger_global_init(TargetHostInterfaceRoster::Listener* listener)
{
	status_t error = TypeHandlerRoster::CreateDefault();
	if (error != B_OK)
		return error;

	error = ImageDebugLoadingStateHandlerRoster::CreateDefault();
	if (error != B_OK)
		return error;

	error = TargetHostInterfaceRoster::CreateDefault(listener);
	if (error != B_OK)
		return error;

	// for now, always create an instance of the local interface
	// by default
	TargetHostInterface* hostInterface;
	TargetHostInterfaceRoster* roster = TargetHostInterfaceRoster::Default();
	error = roster->CreateInterface(roster->InterfaceInfoAt(0), NULL,
		hostInterface);
	if (error != B_OK)
		return error;

	return B_OK;
}


void
debugger_global_uninit()
{
	TargetHostInterfaceRoster::DeleteDefault();
	ImageDebugLoadingStateHandlerRoster::DeleteDefault();
	TypeHandlerRoster::DeleteDefault();
}
