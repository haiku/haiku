/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#include "usb_p.h"


#define TRACE_USB_DEVICE
#ifdef TRACE_USB_DEVICE
#define TRACE(x)	dprintf	x
#else
#define TRACE(x)	/* nothing */
#endif


Device::Device(BusManager *bus, Device *parent, usb_device_descriptor &desc,
	int8 deviceAddress, bool lowSpeed)
{
	fBus = bus;
	fParent = parent;
	fInitOK = false;
	fCurrentConfiguration = NULL;
	fDeviceAddress = deviceAddress;

	TRACE(("USB Device: new device\n"));

	fLock = create_sem(1, "USB Device Lock");
	if (fLock < B_OK) {
		TRACE(("USB Device: could not create semaphore\n"));
		return;
	}

	set_sem_owner(fLock, B_SYSTEM_TEAM);

	fLowSpeed = lowSpeed;
	fDeviceDescriptor = desc;
	fMaxPacketIn[0] = fMaxPacketOut[0] = fDeviceDescriptor.max_packet_size_0; 
	fDefaultPipe = new ControlPipe(this, Pipe::Default,
		lowSpeed ? Pipe::LowSpeed : Pipe::NormalSpeed, 0, fMaxPacketIn[0]);

	// Get the device descriptor
	// We already have a part of it, but we want it all
	status_t status = GetDescriptor(USB_DESCRIPTOR_DEVICE, 0,
		(void *)&fDeviceDescriptor, sizeof(fDeviceDescriptor));

	if (status != sizeof(fDeviceDescriptor)) {
		TRACE(("USB Device: error while getting the device descriptor\n"));
		return;
	}

	TRACE(("full device descriptor for device %d:\n", fDeviceAddress));
	TRACE(("\tlength:..............%d\n", fDeviceDescriptor.length));
	TRACE(("\tdescriptor_type:.....0x%04x\n", fDeviceDescriptor.descriptor_type));
	TRACE(("\tusb_version:.........0x%04x\n", fDeviceDescriptor.usb_version));
	TRACE(("\tdevice_class:........0x%02x\n", fDeviceDescriptor.device_class));
	TRACE(("\tdevice_subclass:.....0x%02x\n", fDeviceDescriptor.device_subclass));
	TRACE(("\tdevice_protocol:.....0x%02x\n", fDeviceDescriptor.device_protocol));
	TRACE(("\tmax_packet_size_0:...%d\n", fDeviceDescriptor.max_packet_size_0));
	TRACE(("\tvendor_id:...........0x%04x\n", fDeviceDescriptor.vendor_id));
	TRACE(("\tproduct_id:..........0x%04x\n", fDeviceDescriptor.product_id));
	TRACE(("\tdevice_version:......0x%04x\n", fDeviceDescriptor.device_version));
	TRACE(("\tmanufacturer:........0x%04x\n", fDeviceDescriptor.manufacturer));
	TRACE(("\tproduct:.............0x%02x\n", fDeviceDescriptor.product));
	TRACE(("\tserial_number:.......0x%02x\n", fDeviceDescriptor.serial_number));
	TRACE(("\tnum_configurations:..%d\n", fDeviceDescriptor.num_configurations));

	// Get the configurations
	fConfigurations = (usb_configuration_descriptor *)malloc(fDeviceDescriptor.num_configurations
		* sizeof(usb_configuration_descriptor));
	if (fConfigurations == NULL) {
		TRACE(("USB Device: out of memory during config creations!\n"));
		return;
	}

	for (int32 i = 0; i < fDeviceDescriptor.num_configurations; i++) {
		size_t size = GetDescriptor(USB_DESCRIPTOR_CONFIGURATION, i,
			(void *)&fConfigurations[i], sizeof(usb_configuration_descriptor));

		if (size != sizeof(usb_configuration_descriptor)) {
			TRACE(("USB Device %d: error fetching configuration %d\n", fDeviceAddress, i));
			return;
		}
	}

	// Set default configuration
	TRACE(("USB Device %d: setting default configuration\n", fDeviceAddress));
	SetConfiguration(0);

	// ToDo: Find drivers for the device
	fInitOK = true;
}


status_t
Device::InitCheck()
{
	if (fInitOK)
		return B_OK;

	return B_ERROR;
}


// Returns the length that was copied (index gives the number of the config)
size_t
Device::GetDescriptor(uint8 descriptorType, uint16 index, void *buffer,
	size_t bufferSize)
{
	size_t actualLength = 0;
	fDefaultPipe->SendRequest(
		USB_REQTYPE_DEVICE_IN | USB_REQTYPE_STANDARD,		// type
		USB_REQUEST_GET_DESCRIPTOR,							// request
		(descriptorType << 8) | index,						// value
		0,													// index
		bufferSize,											// length										
		buffer,												// buffer
		bufferSize,											// buffer length
		&actualLength);										// actual length

	return actualLength;
}			


status_t
Device::SetConfiguration(uint8 value)
{
	if (value >= fDeviceDescriptor.num_configurations)
		return EINVAL;

	status_t result = fDefaultPipe->SendRequest(
		USB_REQTYPE_DEVICE_OUT | USB_REQTYPE_STANDARD,		// type
		USB_REQUEST_SET_CONFIGURATION,						// request
		value,												// value
		0,													// index
		0,													// length										
		NULL,												// buffer
		0,													// buffer length
		NULL);												// actual length

	if (result < B_OK)
		return result;

	// Set current configuration
	fCurrentConfiguration = &fConfigurations[value];
	return B_OK;
}
