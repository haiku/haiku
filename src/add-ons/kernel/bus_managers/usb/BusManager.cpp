/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include "usb_private.h"


BusManager::BusManager(Stack *stack)
	:	fInitOK(false),
		fStack(stack),
		fRootHub(NULL),
		fUSBID((uint32)-1)
{
	mutex_init(&fLock, "usb busmanager lock");

	fRootObject = new(std::nothrow) Object(stack, this);
	if (!fRootObject)
		return;

	// Clear the device map
	for (int32 i = 0; i < 128; i++)
		fDeviceMap[i] = false;
	fDeviceIndex = 0;

	// Set the default pipes to NULL (these will be created when needed)
	for (int32 i = 0; i <= USB_SPEED_MAX; i++)
		fDefaultPipes[i] = NULL;

	fInitOK = true;
}


BusManager::~BusManager()
{
	Lock();
	mutex_destroy(&fLock);
	for (int32 i = 0; i <= USB_SPEED_MAX; i++)
		delete fDefaultPipes[i];
	delete fRootObject;
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
	return (mutex_lock(&fLock) == B_OK);
}


void
BusManager::Unlock()
{
	mutex_unlock(&fLock);
}


int8
BusManager::AllocateAddress()
{
	if (!Lock())
		return -1;

	int8 tries = 127;
	int8 address = fDeviceIndex;
	while (tries-- > 0) {
		if (fDeviceMap[address] == false) {
			fDeviceIndex = (address + 1) % 127;
			fDeviceMap[address] = true;
			Unlock();
			return address + 1;
		}

		address = (address + 1) % 127;
	}

	TRACE_ERROR("the busmanager has run out of device addresses\n");
	Unlock();
	return -1;
}


void
BusManager::FreeAddress(int8 address)
{
	address--;
	if (address < 0)
		return;

	if (!Lock())
		return;

	if (!fDeviceMap[address]) {
		TRACE_ERROR("freeing address %d which was not allocated\n", address);
	}

	fDeviceMap[address] = false;
	Unlock();
}


Device *
BusManager::AllocateDevice(Hub *parent, int8 hubAddress, uint8 hubPort,
	usb_speed speed)
{
	// Check if there is a free entry in the device map (for the device number)
	int8 deviceAddress = AllocateAddress();
	if (deviceAddress < 0) {
		TRACE_ERROR("could not allocate an address\n");
		return NULL;
	}

	TRACE("setting device address to %d\n", deviceAddress);
	ControlPipe *defaultPipe = _GetDefaultPipe(speed);

	if (!defaultPipe) {
		TRACE_ERROR("error getting the default pipe for speed %d\n", speed);
		FreeAddress(deviceAddress);
		return NULL;
	}

	defaultPipe->SetHubInfo(hubAddress, hubPort);

	status_t result = B_ERROR;
	for (int32 i = 0; i < 3; i++) {
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
		TRACE_ERROR("error while setting device address\n");
		FreeAddress(deviceAddress);
		return NULL;
	}

	// Wait a bit for the device to complete addressing
	snooze(USB_DELAY_SET_ADDRESS);

	// Create a temporary pipe with the new address
	ControlPipe pipe(fRootObject);
	pipe.InitCommon(deviceAddress, 0, speed, Pipe::Default, 8, 0, hubAddress,
		hubPort);

	// Get the device descriptor
	// Just retrieve the first 8 bytes of the descriptor -> minimum supported
	// size of any device. It is enough because it includes the device type.

	size_t actualLength = 0;
	usb_device_descriptor deviceDescriptor;

	TRACE("getting the device descriptor\n");
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
		TRACE_ERROR("error while getting the device descriptor\n");
		FreeAddress(deviceAddress);
		return NULL;
	}

	TRACE("short device descriptor for device %d:\n", deviceAddress);
	TRACE("\tlength:..............%d\n", deviceDescriptor.length);
	TRACE("\tdescriptor_type:.....0x%04x\n", deviceDescriptor.descriptor_type);
	TRACE("\tusb_version:.........0x%04x\n", deviceDescriptor.usb_version);
	TRACE("\tdevice_class:........0x%02x\n", deviceDescriptor.device_class);
	TRACE("\tdevice_subclass:.....0x%02x\n", deviceDescriptor.device_subclass);
	TRACE("\tdevice_protocol:.....0x%02x\n", deviceDescriptor.device_protocol);
	TRACE("\tmax_packet_size_0:...%d\n", deviceDescriptor.max_packet_size_0);

	// Create a new instance based on the type (Hub or Device)
	if (deviceDescriptor.device_class == 0x09) {
		TRACE("creating new hub\n");
		Hub *hub = new(std::nothrow) Hub(parent, hubAddress, hubPort,
			deviceDescriptor, deviceAddress, speed, false);
		if (!hub) {
			TRACE_ERROR("no memory to allocate hub\n");
			FreeAddress(deviceAddress);
			return NULL;
		}

		if (hub->InitCheck() < B_OK) {
			TRACE_ERROR("hub failed init check\n");
			FreeAddress(deviceAddress);
			delete hub;
			return NULL;
		}

		return (Device *)hub;
	}

	TRACE("creating new device\n");
	Device *device = new(std::nothrow) Device(parent, hubAddress, hubPort,
		deviceDescriptor, deviceAddress, speed, false);
	if (!device) {
		TRACE_ERROR("no memory to allocate device\n");
		FreeAddress(deviceAddress);
		return NULL;
	}

	if (device->InitCheck() < B_OK) {
		TRACE_ERROR("device failed init check\n");
		FreeAddress(deviceAddress);
		delete device;
		return NULL;
	}

	return device;
}


void
BusManager::FreeDevice(Device *device)
{
	FreeAddress(device->DeviceAddress());
	delete device;
}


status_t
BusManager::Start()
{
	fStack->AddBusManager(this);
	fUSBID = fStack->IndexOfBusManager(this);
	return B_OK;
}


status_t
BusManager::Stop()
{
	return B_OK;
}


status_t
BusManager::StartDebugTransfer(Transfer *transfer)
{
	// virtual function to be overridden
	return B_UNSUPPORTED;
}


status_t
BusManager::CheckDebugTransfer(Transfer *transfer)
{
	// virtual function to be overridden
	return B_UNSUPPORTED;
}


void
BusManager::CancelDebugTransfer(Transfer *transfer)
{
	// virtual function to be overridden
}


status_t
BusManager::SubmitTransfer(Transfer *transfer)
{
	// virtual function to be overridden
	return B_ERROR;
}


status_t
BusManager::CancelQueuedTransfers(Pipe *pipe, bool force)
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
BusManager::_GetDefaultPipe(usb_speed speed)
{
	if (!Lock())
		return NULL;

	if (fDefaultPipes[speed] == NULL) {
		fDefaultPipes[speed] = new(std::nothrow) ControlPipe(fRootObject);
		fDefaultPipes[speed]->InitCommon(0, 0, speed, Pipe::Default, 8, 0, 0, 0);
	}

	if (!fDefaultPipes[speed]) {
		TRACE_ERROR("failed to allocate default pipe for speed %d\n", speed);
	}

	Unlock();
	return fDefaultPipes[speed];
}
