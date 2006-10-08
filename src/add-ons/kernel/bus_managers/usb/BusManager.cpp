/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include "usb_p.h"


BusManager::BusManager(Stack *stack)
	:	fInitOK(false),
		fRootHub(NULL)
{
	if (benaphore_init(&fLock, "usb busmanager lock") < B_OK) {
		TRACE_ERROR(("usb BusManager: failed to create busmanager lock\n"));
		return;
	}

	fRootObject = new(std::nothrow) Object(stack, this);
	if (!fRootObject)
		return;

	// Clear the device map
	for (int32 i = 0; i < 128; i++)
		fDeviceMap[i] = false;

	// Set the default pipes to NULL (these will be created when needed)
	for (int32 i = 0; i <= USB_SPEED_MAX; i++)
		fDefaultPipes[i] = 0;

	fInitOK = true;
}


BusManager::~BusManager()
{
	Lock();
	benaphore_destroy(&fLock);
	for (int32 i = 0; i <= USB_SPEED_MAX; i++)
		if (fDefaultPipes[i] != 0)
			delete fDefaultPipes[i];
}


status_t
BusManager::InitCheck()
{
	if (fInitOK)
		return B_OK;

	return B_ERROR;
}


bool
BusManager::Lock()
{
	return (benaphore_lock(&fLock) == B_OK);
}


void
BusManager::Unlock()
{
	benaphore_unlock(&fLock);
}


int8
BusManager::AllocateAddress()
{
	if (!Lock())
		return -1;

	int8 deviceAddress = -1;
	for (int32 i = 1; i < 128; i++) {
		if (fDeviceMap[i] == false) {
			deviceAddress = i;
			fDeviceMap[i] = true;
			break;
		}
	}

	Unlock();
	return deviceAddress;
}


Device *
BusManager::AllocateNewDevice(Hub *parent, usb_speed speed)
{
	// Check if there is a free entry in the device map (for the device number)
	int8 deviceAddress = AllocateAddress();
	if (deviceAddress < 0) {
		TRACE_ERROR(("usb BusManager::AllocateNewDevice(): could not get a new address\n"));
		return NULL;
	}

	TRACE(("usb BusManager::AllocateNewDevice(): setting device address to %d\n", deviceAddress));
	ControlPipe *defaultPipe = GetDefaultPipe(speed);
	
	if (!defaultPipe) {
		TRACE(("usb BusManager::AllocateNewDevice(): Error getting the default pipe for speed %d\n", (int)speed));
		return NULL;
	}

	status_t result = B_ERROR;
	for (int32 i = 0; i < 15; i++) {
		// Set the address of the device USB 1.1 spec p202
		result = defaultPipe->SendRequest(
			USB_REQTYPE_STANDARD | USB_REQTYPE_DEVICE_OUT,	// type
			USB_REQUEST_SET_ADDRESS,						// request
			deviceAddress,									// value
			0,												// index
			0,												// length
			NULL,											// buffer
			0,												// buffer length
			NULL);											// actual length

		if (result >= B_OK)
			break;

		snooze(USB_DELAY_SET_ADDRESS_RETRY);
	}

	if (result < B_OK) {
		TRACE_ERROR(("usb BusManager::AllocateNewDevice(): error while setting device address\n"));
		return NULL;
	}

	// Wait a bit for the device to complete addressing
	snooze(USB_DELAY_SET_ADDRESS);

	// Create a temporary pipe with the new address
	ControlPipe pipe(parent, deviceAddress, 0, speed, 8);

	// Get the device descriptor
	// Just retrieve the first 8 bytes of the descriptor -> minimum supported
	// size of any device. It is enough because it includes the device type.

	size_t actualLength = 0;
	usb_device_descriptor deviceDescriptor;

	TRACE(("usb BusManager::AllocateNewDevice(): getting the device descriptor\n"));
	pipe.SendRequest(
		USB_REQTYPE_DEVICE_IN | USB_REQTYPE_STANDARD,		// type
		USB_REQUEST_GET_DESCRIPTOR,							// request
		USB_DESCRIPTOR_DEVICE << 8,							// value
		0,													// index
		8,													// length										
		(void *)&deviceDescriptor,							// buffer
		8,													// buffer length
		&actualLength);										// actual length

	if (actualLength != 8) {
		TRACE_ERROR(("usb BusManager::AllocateNewDevice(): error while getting the device descriptor\n"));
		return NULL;
	}

	TRACE(("short device descriptor for device %d:\n", deviceAddress));
	TRACE(("\tlength:..............%d\n", deviceDescriptor.length));
	TRACE(("\tdescriptor_type:.....0x%04x\n", deviceDescriptor.descriptor_type));
	TRACE(("\tusb_version:.........0x%04x\n", deviceDescriptor.usb_version));
	TRACE(("\tdevice_class:........0x%02x\n", deviceDescriptor.device_class));
	TRACE(("\tdevice_subclass:.....0x%02x\n", deviceDescriptor.device_subclass));
	TRACE(("\tdevice_protocol:.....0x%02x\n", deviceDescriptor.device_protocol));
	TRACE(("\tmax_packet_size_0:...%d\n", deviceDescriptor.max_packet_size_0));

	// Create a new instance based on the type (Hub or Device)
	if (deviceDescriptor.device_class == 0x09) {
		TRACE(("usb BusManager::AllocateNewDevice(): creating new hub\n"));
		Hub *hub = new(std::nothrow) Hub(parent, deviceDescriptor,
			deviceAddress, speed);
		if (!hub) {
			TRACE_ERROR(("usb BusManager::AllocateNewDevice(): no memory to allocate hub\n"));
			return NULL;
		}

		if (hub->InitCheck() < B_OK) {
			TRACE_ERROR(("usb BusManager::AllocateNewDevice(): hub failed init check\n"));
			delete hub;
			return NULL;
		}

		return (Device *)hub;
	}

	TRACE(("usb BusManager::AllocateNewDevice(): creating new device\n"));
	Device *device = new(std::nothrow) Device(parent, deviceDescriptor,
		deviceAddress, speed);
	if (!device) {
		TRACE_ERROR(("usb BusManager::AllocateNewDevice(): no memory to allocate device\n"));
		return NULL;
	}

	if (device->InitCheck() < B_OK) {
		TRACE_ERROR(("usb BusManager::AllocateNewDevice(): device failed init check\n"));
		delete device;
		return NULL;
	}

	return device;
}


status_t
BusManager::Start()
{
	return B_OK;
}


status_t
BusManager::Stop()
{
	return B_OK;
}


status_t
BusManager::SubmitTransfer(Transfer *transfer)
{
	// virtual function to be overridden
	return B_ERROR;
}


status_t
BusManager::NotifyPipeChange(Pipe *pipe, usb_change change)
{
	// virtual function to be overridden
	return B_ERROR;
}

ControlPipe *
BusManager::GetDefaultPipe(usb_speed speed)
{
	if (fDefaultPipes[(int)speed] == 0) {
		fDefaultPipes[(int)speed] = new(std::nothrow) ControlPipe(fRootObject,
					0, 0, (usb_speed)speed, 8);
		if (!fDefaultPipes[(int)speed]) {
			TRACE_ERROR(("usb BusManager: failed to allocate default pipe\n"));
			return 0;
		}
	}
	
	return fDefaultPipes[(int)speed];
}
