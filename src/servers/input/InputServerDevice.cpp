/*
 * Copyright 2002-2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "InputServer.h"

#include <InputServerDevice.h>


BInputServerDevice::BInputServerDevice()
{
	fOwner = new _BDeviceAddOn_(this);
}


BInputServerDevice::~BInputServerDevice()
{
	CALLED();

	gInputServer->UnregisterDevices(*this);
	delete fOwner;
}


status_t
BInputServerDevice::InitCheck()
{
	return B_OK;
}


status_t
BInputServerDevice::SystemShuttingDown()
{
	return B_OK;
}


status_t
BInputServerDevice::Start(const char* device, void* cookie)
{
	return B_OK;
}


status_t
BInputServerDevice::Stop(const char* device, void* cookie)
{
	return B_OK;
}


status_t
BInputServerDevice::Control(const char* device, void* cookie,
	uint32 code, BMessage* message)
{
	return B_OK;
}


status_t
BInputServerDevice::RegisterDevices(input_device_ref** devices)
{
	CALLED();
	return gInputServer->RegisterDevices(*this, devices);
}


status_t
BInputServerDevice::UnregisterDevices(input_device_ref** devices)
{
    CALLED();
    // TODO: is this function supposed to remove devices that do not belong to this object?
    //	(at least that's what the previous implementation allowed for)
	return gInputServer->UnregisterDevices(*this, devices);
}


status_t
BInputServerDevice::EnqueueMessage(BMessage* message)
{
	return gInputServer->EnqueueDeviceMessage(message);
}


status_t
BInputServerDevice::StartMonitoringDevice(const char* device)
{
	CALLED();
	PRINT(("StartMonitoringDevice %s\n", device));

	return InputServer::gDeviceManager.StartMonitoringDevice(fOwner, device);
}


status_t
BInputServerDevice::StopMonitoringDevice(const char* device)
{
	CALLED();
	return InputServer::gDeviceManager.StopMonitoringDevice(fOwner, device);
}


void BInputServerDevice::_ReservedInputServerDevice1() {}
void BInputServerDevice::_ReservedInputServerDevice2() {}
void BInputServerDevice::_ReservedInputServerDevice3() {}
void BInputServerDevice::_ReservedInputServerDevice4() {}

