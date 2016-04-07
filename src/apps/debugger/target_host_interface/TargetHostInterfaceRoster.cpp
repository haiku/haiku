/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "TargetHostInterfaceRoster.h"

#include <new>

#include <AutoDeleter.h>

#include "LocalTargetHostInterfaceInfo.h"
#include "TargetHostInterface.h"
#include "TargetHostInterfaceInfo.h"


/*static*/ TargetHostInterfaceRoster*
	TargetHostInterfaceRoster::sDefaultInstance = NULL;


TargetHostInterfaceRoster::TargetHostInterfaceRoster()
	:
	fLock(),
	fInterfaceInfos(20, false),
	fActiveInterfaces(20, false)
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
TargetHostInterfaceRoster::CreateDefault()
{
	if (sDefaultInstance != NULL)
		return B_OK;

	TargetHostInterfaceRoster* roster
		= new(std::nothrow) TargetHostInterfaceRoster;
	if (roster == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<TargetHostInterfaceRoster> rosterDeleter(roster);

	status_t error = roster->Init();
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
TargetHostInterfaceRoster::Init()
{
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
		if (!fInterfaceInfos.AddItem(info)) \
			return B_NO_MEMORY; \
		interfaceReference.Detach();

	REGISTER_INTERFACE_INFO(Local)

	return B_OK;
}
