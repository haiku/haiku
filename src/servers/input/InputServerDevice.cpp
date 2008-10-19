/*
 * Copyright 2002-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <InputServerDevice.h>

#include <new>

#include <Directory.h>
#include <Path.h>

#include "InputServer.h"


DeviceAddOn::DeviceAddOn(BInputServerDevice *device)
	:
	fDevice(device)
{
}


DeviceAddOn::~DeviceAddOn()
{
	while (const char* path = fMonitoredPaths.PathAt(0)) {
		gInputServer->AddOnManager()->StopMonitoringDevice(this, path);
	}
}


bool
DeviceAddOn::HasPath(const char* path) const
{
	return fMonitoredPaths.HasPath(path);
}


status_t
DeviceAddOn::AddPath(const char* path)
{
	return fMonitoredPaths.AddPath(path);
}


status_t
DeviceAddOn::RemovePath(const char* path)
{
	return fMonitoredPaths.RemovePath(path);
}


int32
DeviceAddOn::CountPaths() const
{
	return fMonitoredPaths.CountPaths();
}


//	#pragma mark -


BInputServerDevice::BInputServerDevice()
{
	fOwner = new(std::nothrow) DeviceAddOn(this);
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
	if (fOwner == NULL)
		return B_NO_MEMORY;
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

	return gInputServer->AddOnManager()->StartMonitoringDevice(fOwner, device);
}


status_t
BInputServerDevice::StopMonitoringDevice(const char* device)
{
	return gInputServer->AddOnManager()->StopMonitoringDevice(fOwner, device);
}


status_t
BInputServerDevice::AddDevices(const char* path)
{
	BDirectory directory;
	status_t status = directory.SetTo(path);
	if (status != B_OK)
		return status;

	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		BPath entryPath(&entry);

		if (entry.IsDirectory()) {
			AddDevices(entryPath.Path());
		} else {
			BMessage added(B_NODE_MONITOR);
			added.AddInt32("opcode", B_ENTRY_CREATED);
			added.AddString("path", entryPath.Path());

			Control(NULL, NULL, B_INPUT_DEVICE_ADDED, &added);
		}
	}

	return B_OK;
}


void BInputServerDevice::_ReservedInputServerDevice1() {}
void BInputServerDevice::_ReservedInputServerDevice2() {}
void BInputServerDevice::_ReservedInputServerDevice3() {}
void BInputServerDevice::_ReservedInputServerDevice4() {}
