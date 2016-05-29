/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "LocalTargetHostInterfaceInfo.h"

#include "LocalTargetHostInterface.h"


LocalTargetHostInterfaceInfo::LocalTargetHostInterfaceInfo()
	:
	TargetHostInterfaceInfo("Local")
{
}


LocalTargetHostInterfaceInfo::~LocalTargetHostInterfaceInfo()
{
}


status_t
LocalTargetHostInterfaceInfo::Init()
{
	return B_OK;
}


bool
LocalTargetHostInterfaceInfo::IsLocal() const
{
	return true;
}


bool
LocalTargetHostInterfaceInfo::IsConfigured(Settings* settings) const
{
	return true;
}


SettingsDescription*
LocalTargetHostInterfaceInfo::GetSettingsDescription() const
{
	// the local interface requires no configuration, therefore
	// it returns no settings description, and has no real work
	// to do as far as settings validation is concerned.
	return NULL;
}


status_t
LocalTargetHostInterfaceInfo::CreateInterface(Settings* settings,
	TargetHostInterface*& _interface) const
{
	LocalTargetHostInterface* interface
		= new(std::nothrow) LocalTargetHostInterface;
	if (interface == NULL)
		return B_NO_MEMORY;

	status_t error = interface->Init(settings);
	if (error != B_OK) {
		delete interface;
		return error;
	}

	_interface = interface;
	return B_OK;
}

