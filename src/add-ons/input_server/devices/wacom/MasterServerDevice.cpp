/*
 * Copyright 2005-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#include "MasterServerDevice.h"

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Directory.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <NodeMonitor.h>
#include <OS.h>
#include <Path.h>
#include <Screen.h>
#include <View.h>
#include <File.h>

#include "PointingDevice.h"
#include "PointingDeviceFactory.h"

#define DEFAULT_CLICK_SPEED 250000

static const char* kWatchFolder			= "input/wacom/usb";
static const char* kDeviceFolder		= "/dev/input/wacom/usb";

//static const char* kPS2MouseThreadName	= "PS/2 Mouse";

// instantiate_input_device
//
// this is where it all starts make sure this function is exported!
BInputServerDevice*
instantiate_input_device()
{
	return (new MasterServerDevice());
}

// constructor
MasterServerDevice::MasterServerDevice()
	: BInputServerDevice(),
	  fDevices(1),
	  fActive(false),
	  fDblClickSpeed(DEFAULT_CLICK_SPEED),
	  fPS2DisablerThread(B_ERROR),
	  fDeviceLock("device list lock")
{
	get_mouse_speed(&fSpeed);
	get_mouse_acceleration(&fAcceleration);
	get_click_speed(&fDblClickSpeed);
	_CalculateAccelerationTable();

	if (_LockDevices()) {
		// do an initial scan of the devfs folder, after that, we should receive
		// node monitor messages for hotplugging support
		_SearchDevices();
	
		fActive = true;
	
		for (int32 i = 0; PointingDevice* device = (PointingDevice*)fDevices.ItemAt(i); i++) {
			device->Start();
		}
		_UnlockDevices();
	}

	StartMonitoringDevice(kWatchFolder);
}

// destructor
MasterServerDevice::~MasterServerDevice()
{
	// cleanup
	_StopAll();
	if (_LockDevices()) {
		while (PointingDevice* device = (PointingDevice*)fDevices.RemoveItem((int32)0))
			delete device;
		_UnlockDevices();
	}
}

// InitCheck
status_t
MasterServerDevice::InitCheck()
{
	return BInputServerDevice::InitCheck();
}

// SystemShuttingDown
status_t
MasterServerDevice::SystemShuttingDown()
{
	// the devices should be stopped anyways,
	// but this is just defensive programming.
	// the pointing devices will have spawned polling threads,
	// we make sure that we block until every thread has bailed out.

	_StopAll();

	PRINT(("---------------------------------\n\n"));

	return (BInputServerDevice::SystemShuttingDown());
}

// Start
//
// start generating events
status_t
MasterServerDevice::Start(const char* device, void* cookie)
{
	// TODO: make this configurable
//	_StartPS2DisablerThread();

	return B_OK;
}

// Stop
status_t
MasterServerDevice::Stop(const char* device, void* cookie)
{
	// stop generating events
	fActive = false;

	_StopAll();

//	_StopPS2DisablerThread();

	return StopMonitoringDevice(kWatchFolder);
}

// Control
status_t
MasterServerDevice::Control(const char* device, void* cookie, uint32 code, BMessage* message)
{
	// respond to changes in the system
	switch (code) {
		case B_MOUSE_SPEED_CHANGED:
			get_mouse_speed(&fSpeed);
			_CalculateAccelerationTable();
			break;
		case B_CLICK_SPEED_CHANGED:
			get_click_speed(&fDblClickSpeed);
			break;
		case B_MOUSE_ACCELERATION_CHANGED:
			get_mouse_acceleration(&fAcceleration);
			_CalculateAccelerationTable();
			break;
		case B_NODE_MONITOR:
			_HandleNodeMonitor(message);
			break;
		default:
			BInputServerDevice::Control(device, cookie, code, message);
			break;
	}
	return B_OK;
}

// #pragma mark -

// _SearchDevices
void
MasterServerDevice::_SearchDevices()
{
	if (_LockDevices()) {
		// construct a PointingDevice for every devfs entry we find
		BDirectory dir(kDeviceFolder);
		if (dir.InitCheck() >= B_OK) {
			// traverse the contents of the folder to see if the
			// entry of that device still exists
			entry_ref ref;
			while (dir.GetNextRef(&ref) >= B_OK) {
				PRINT(("examining devfs entry '%s'\n", ref.name));
				// don't add the control device
				if (strcmp(ref.name, "control") != 0) {
					BPath path(&ref);
					if (path.InitCheck() >= B_OK) {
						// add the device
						_AddDevice(path.Path());
					}
				}
			}
		} else
			PRINT(("folder '%s' not found\n", kDeviceFolder));
	
		PRINT(("done examing devfs\n"));
		_UnlockDevices();
	}
}

// _StopAll
void
MasterServerDevice::_StopAll()
{
	if (_LockDevices()) {
		for (int32 i = 0; PointingDevice* device
				= (PointingDevice*)fDevices.ItemAt(i); i++)
			device->Stop();
		_UnlockDevices();
	}
}

// _AddDevice
void
MasterServerDevice::_AddDevice(const char* path)
{
	if (_LockDevices()) {
		// get the device from the factory
		PointingDevice* device = PointingDeviceFactory::DeviceFor(this, path);
		// add it to our list
		if (device && device->InitCheck() >= B_OK
			&& fDevices.AddItem((void*)device)) {
			PRINT(("pointing device added (%s)\n", path));
			// start device polling only if we're started
			if (fActive)
				device->Start();
			input_device_ref device = { (char *)"Wacom Tablet",
				B_POINTING_DEVICE, (void*)this };
			input_device_ref* deviceList[2] = { &device, NULL };
			RegisterDevices(deviceList);
		} else {
	
			PRINT(("pointing device not added (%s)\n", path));
			if (device) {
				PRINT(("  vendor: %0*x, product: %0*x\n", 4, device->VendorID(),
					4, device->ProductID()));
			}
	
			delete device;
		}
		_UnlockDevices();
	}
}

// _HandleNodeMonitor
void
MasterServerDevice::_HandleNodeMonitor(BMessage* message)
{
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) < B_OK)
		return;

	switch (opcode) {
		case B_ENTRY_CREATED:
			// extract info to create an entry_ref structure
			const char* name;
			ino_t directory;
			dev_t device;
			if (message->FindString("name", &name) >= B_OK
				&& strcmp(name, "control") != 0
				&& message->FindInt64("directory", (int64*)&directory) >= B_OK
				&& message->FindInt32("device", (int32*)&device) >= B_OK) {
				// make a path from that info
				entry_ref ref(device, directory, name);
				BPath path(&ref);
				if (path.InitCheck() >= B_OK) {
					// add the device
					_AddDevice(path.Path());
				}
			}
			break;
		case B_ENTRY_REMOVED: {
			// I cannot figure out how to see if this is actually
			// the directory that we're node monitoring, and even if it is,
			// we would have to look at the directories contents to
			// see which PointingDevice we might want to remove
			BDirectory dir(kDeviceFolder);
			if (dir.InitCheck() >= B_OK) {
				// for each device in our list, see if the corresponding
				// entry in the device folder still exists
				for (int32 i = 0; PointingDevice* pointingDevice
						= (PointingDevice*)fDevices.ItemAt(i); i++) {
					// traverse the contents of the folder
					// if the entry for this device is there,
					// we can abort the search
					entry_ref ref;
					dir.Rewind();
					bool found = false;
					while (dir.GetNextRef(&ref) >= B_OK) {
						BPath path(&ref);
						if (strcmp(pointingDevice->DevicePath(),
								path.Path()) == 0) {
							found = true;
							break;
						}
					}
					// remove the device if the devfs entry was not found
					if (!found) {

						PRINT(("removing device '%s'\n", pointingDevice->DevicePath()));

						if (_LockDevices()) {
							if (fDevices.RemoveItem((void*)pointingDevice)) {
								
								delete pointingDevice;
								i--;
							}
							_UnlockDevices();
						}
					}
				}
			}
			break;
		}
	}
}

// _CalculateAccelerationTable
//
// calculates the acceleration table. This is recalculated anytime we find out that
// the current acceleration or speed has changed.
void
MasterServerDevice::_CalculateAccelerationTable()
{
	// This seems to work alright.
	for (int x = 1; x <= 128; x++){
		fAccelerationTable[x] = (x / (128 * (1 - (((float)fSpeed + 8192) / 262144))))
			* (x / (128 * (1 - (((float)fSpeed + 8192) / 262144))))
			+ ((((float)fAcceleration + 4000) / 262144)
			* (x / (128 * (1 - (((float)fSpeed + 8192) / 262144)))));
	}
	
	// sets the bottom of the table to be the inverse of the first half.
	// position 0 -> 128 positive movement, 255->129 negative movement
	for (int x = 255; x > 128; x--){
		fAccelerationTable[x] = -(fAccelerationTable[(255 - x)]);
	}
	
	// Location 0 will be 0.000, which is unacceptable, otherwise, single step moves are lost
	// To work around this, we'll make it 1/2 of the smallest increment.
	fAccelerationTable[0] = fAccelerationTable[1] / 2;
	fAccelerationTable[255] = -fAccelerationTable[0];
}

// _ps2_disabler
//
// find the PS/2 device thread and suspend its execution
// TODO: make this configurable
// In case you're wondering... this behaviour is useful for notebook
// users who are annoyed by accidentally hitting their touchpad while
// typing. "Turning it off entirely" by suspending the PS/2 thread is
// just a compfort thing, but should of course be made configurable...
//int32
//MasterServerDevice::_ps2_disabler(void* cookie)
//{
//	MasterServerDevice* master = (MasterServerDevice*)cookie;
//	
//	thread_id haltedThread = B_ERROR;
//
//	while (master->fActive) {
//		// search for at least one running device
//		bool suspendPS2 = false;
//		if (master->_LockDevices()) {
//			for (int32 i = 0; PointingDevice* device = (PointingDevice*)master->fDevices.ItemAt(i); i++) {
//				if (device->DisablePS2()) {
//					suspendPS2 = true;
//					break;
//				}
//			}
//			master->_UnlockDevices();
//		}
//
//		if (suspendPS2){
//			// find and suspend PS/2 thread (if not already done)
//			if (haltedThread < B_OK) {
//				haltedThread = find_thread(kPS2MouseThreadName);
//				if (haltedThread >= B_OK) {
//					suspend_thread(haltedThread);
//				}
//			}
//		} else {
//			// reenable PS/2 thread
//			if (haltedThread >= B_OK) {
//				resume_thread(haltedThread);
//				haltedThread = B_ERROR;
//			}
//		}
//
//		// sleep a little while
//		snooze(2000000);
//	}
//
//	// reenable PS/2 thread in any case before we die
//	if (haltedThread >= B_OK) {
//		resume_thread(haltedThread);
//	}
//
//	return B_OK;
//}
//
//// _StartPS2DisablerThread
//void
//MasterServerDevice::_StartPS2DisablerThread()
//{
//	if (fPS2DisablerThread < B_OK) {
//		fPS2DisablerThread = spawn_thread(_ps2_disabler, "PS/2 Mouse disabler", B_LOW_PRIORITY, this);
//		if (fPS2DisablerThread >= B_OK)
//			resume_thread(fPS2DisablerThread);
//	}
//}
//
//// _StopPS2DisablerThread
//void
//MasterServerDevice::_StopPS2DisablerThread()
//{
//	if (fPS2DisablerThread >= B_OK) {
//		status_t err;
//		wait_for_thread(fPS2DisablerThread, &err);
//	}
//	fPS2DisablerThread = B_ERROR;
//}

// _LockDevices
bool
MasterServerDevice::_LockDevices()
{
	return fDeviceLock.Lock();
}

// _UnlockDevices
void
MasterServerDevice::_UnlockDevices()
{
	fDeviceLock.Unlock();
}

