/*
 * Copyright 2016-2017, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "TargetHostInterfaceRoster.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "LocalTargetHostInterfaceInfo.h"
#include "NetworkTargetHostInterfaceInfo.h"
#include "TargetHostInterfaceInfo.h"


/*static*/ TargetHostInterfaceRoster*
	TargetHostInterfaceRoster::sDefaultInstance = NULL;


TargetHostInterfaceRoster::TargetHostInterfaceRoster()
	:
	TargetHostInterface::Listener(),
	fLock(),
	fRunningTeamDebuggers(0),
	fInterfaceInfos(20, false),
	fActiveInterfaces(20, false),
	fListener(NULL)
{
}


TargetHostInterfaceRoster::~TargetHostInterfaceRoster()
{
}


/*static*/ TargetHostInterfaceRoster*
TargetHostInterfaceRoster::Default()
{
	return sDefaultInstance;
}


/*static*/ status_t
TargetHostInterfaceRoster::CreateDefault(Listener* listener)
{
	if (sDefaultInstance != NULL)
		return B_OK;

	TargetHostInterfaceRoster* roster
		= new(std::nothrow) TargetHostInterfaceRoster;
	if (roster == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<TargetHostInterfaceRoster> rosterDeleter(roster);

	status_t error = roster->Init(listener);
	if (error != B_OK)
		return error;

	error = roster->RegisterInterfaceInfos();
	if (error != B_OK)
		return error;

	sDefaultInstance = rosterDeleter.Detach();
	return B_OK;
}


/*static*/ void
TargetHostInterfaceRoster::DeleteDefault()
{
	TargetHostInterfaceRoster* roster = sDefaultInstance;
	sDefaultInstance = NULL;
	delete roster;
}


status_t
TargetHostInterfaceRoster::Init(Listener* listener)
{
	fListener = listener;
	return fLock.InitCheck();
}


status_t
TargetHostInterfaceRoster::RegisterInterfaceInfos()
{
	TargetHostInterfaceInfo* info = NULL;
	BReference<TargetHostInterfaceInfo> interfaceReference;

	#undef REGISTER_INTERFACE_INFO
	#define REGISTER_INTERFACE_INFO(type) \
		info = new(std::nothrow) type##TargetHostInterfaceInfo; \
		if (info == NULL) \
			return B_NO_MEMORY; \
		interfaceReference.SetTo(info, true); \
		if (info->Init() != B_OK) \
			return B_NO_MEMORY; \
		if (!fInterfaceInfos.AddItem(info)) \
			return B_NO_MEMORY; \
		interfaceReference.Detach();

	REGISTER_INTERFACE_INFO(Local)
	REGISTER_INTERFACE_INFO(Network)

	return B_OK;
}


int32
TargetHostInterfaceRoster::CountInterfaceInfos() const
{
	return fInterfaceInfos.CountItems();
}


TargetHostInterfaceInfo*
TargetHostInterfaceRoster::InterfaceInfoAt(int32 index) const
{
	return fInterfaceInfos.ItemAt(index);
}


status_t
TargetHostInterfaceRoster::CreateInterface(TargetHostInterfaceInfo* info,
	Settings* settings, TargetHostInterface*& _interface)
{
	// TODO: this should eventually verify that an active interface with
	// matching settings/type doesn't already exist, and if so, return that
	// directly rather than instantiating a new one, since i.e. the interface
	// for the local host only requires one instance.
	AutoLocker<TargetHostInterfaceRoster> locker(this);
	TargetHostInterface* interface;
	status_t error = info->CreateInterface(settings, interface);
	if (error != B_OK)
		return error;

	error = interface->Run();
	if (error < B_OK || !fActiveInterfaces.AddItem(interface)) {
		delete interface;
		return B_NO_MEMORY;
	}

	interface->AddListener(this);
	_interface = interface;
	return B_OK;
}


int32
TargetHostInterfaceRoster::CountActiveInterfaces() const
{
	return fActiveInterfaces.CountItems();
}


TargetHostInterface*
TargetHostInterfaceRoster::ActiveInterfaceAt(int32 index) const
{
	return fActiveInterfaces.ItemAt(index);
}


void
TargetHostInterfaceRoster::TeamDebuggerStarted(TeamDebugger* debugger)
{
	fRunningTeamDebuggers++;
	fListener->TeamDebuggerCountChanged(fRunningTeamDebuggers);
}


void
TargetHostInterfaceRoster::TeamDebuggerQuit(TeamDebugger* debugger)
{
	fRunningTeamDebuggers--;
	fListener->TeamDebuggerCountChanged(fRunningTeamDebuggers);
}


void
TargetHostInterfaceRoster::TargetHostInterfaceQuit(
	TargetHostInterface* interface)
{
	AutoLocker<TargetHostInterfaceRoster> locker(this);
	fActiveInterfaces.RemoveItem(interface);
}


// #pragma mark - TargetHostInterfaceRoster::Listener


TargetHostInterfaceRoster::Listener::~Listener()
{
}


void
TargetHostInterfaceRoster::Listener::TeamDebuggerCountChanged(int32 count)
{
}
