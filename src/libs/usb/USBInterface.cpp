/*
 * Copyright 2007-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#include <USBKit.h>
#include <usb_raw.h>
#include <unistd.h>
#include <string.h>


BUSBInterface::BUSBInterface(BUSBConfiguration *config, uint32 index, int rawFD)
	:	fConfiguration(config),
		fIndex(index),
		fRawFD(rawFD),
		fEndpoints(NULL),
		fInterfaceString(NULL)
{
	_UpdateDescriptorAndEndpoints();
}


BUSBInterface::~BUSBInterface()
{
	delete[] fInterfaceString;
	for (int32 i = 0; i < fDescriptor.num_endpoints; i++)
		delete fEndpoints[i];
	delete[] fEndpoints;
}


uint32
BUSBInterface::Index() const
{
	return fIndex;
}


const BUSBConfiguration *
BUSBInterface::Configuration() const
{
	return fConfiguration;
}


const BUSBDevice *
BUSBInterface::Device() const
{
	return fConfiguration->Device();
}


uint8
BUSBInterface::Class() const
{
	return fDescriptor.interface_class;
}


uint8
BUSBInterface::Subclass() const
{
	return fDescriptor.interface_subclass;
}


uint8
BUSBInterface::Protocol() const
{
	return fDescriptor.interface_protocol;
}


const char *
BUSBInterface::InterfaceString() const
{
	if (fDescriptor.interface == 0)
		return "";

	if (fInterfaceString)
		return fInterfaceString;

	fInterfaceString = Device()->DecodeStringDescriptor(fDescriptor.interface);
	if (!fInterfaceString) {
		fInterfaceString = new char[1];
		fInterfaceString[0] = 0;
	}

	return fInterfaceString;
}


const usb_interface_descriptor *
BUSBInterface::Descriptor() const
{
	return &fDescriptor;
}


status_t
BUSBInterface::OtherDescriptorAt(uint32 index, usb_descriptor *descriptor,
	size_t length) const
{
	if (length > 0 && descriptor == NULL)
		return B_BAD_VALUE;

	raw_command command;
	command.generic.descriptor = descriptor;
	command.generic.config_index = fConfiguration->Index();
	command.generic.interface_index = fIndex;
	command.generic.length = length;
	command.generic.generic_index = index;
	if (ioctl(fRawFD, RAW_COMMAND_GET_GENERIC_DESCRIPTOR, &command, sizeof(command))
		|| command.generic.status != RAW_STATUS_SUCCESS) {
		return B_ERROR;
	}

	return B_OK;
}


uint32
BUSBInterface::CountEndpoints() const
{
	return fDescriptor.num_endpoints;
}


const BUSBEndpoint *
BUSBInterface::EndpointAt(uint32 index) const
{
	if (index >= fDescriptor.num_endpoints)
		return NULL;

	return fEndpoints[index];
}


uint32
BUSBInterface::CountAlternates() const
{
	uint32 alternateCount;
	raw_command command;
	command.alternate.alternate_count = &alternateCount;
	command.alternate.config_index = fConfiguration->Index();
	command.alternate.interface_index = fIndex;
	if (ioctl(fRawFD, RAW_COMMAND_GET_ALT_INTERFACE_COUNT, &command, sizeof(command))
		|| command.alternate.status != RAW_STATUS_SUCCESS) {
		return 1;
	}

	return alternateCount;
}


status_t
BUSBInterface::SetAlternate(uint32 alternateIndex)
{
	raw_command command;
	command.alternate.config_index = fConfiguration->Index();
	command.alternate.interface_index = fIndex;
	command.alternate.alternate_index = alternateIndex;
	if (ioctl(fRawFD, RAW_COMMAND_SET_ALT_INTERFACE, &command, sizeof(command))
		|| command.alternate.status != RAW_STATUS_SUCCESS) {
		return B_ERROR;
	}

	_UpdateDescriptorAndEndpoints();
	return B_OK;
}


void
BUSBInterface::_UpdateDescriptorAndEndpoints()
{
	raw_command command;
	command.interface.descriptor = &fDescriptor;
	command.interface.config_index = fConfiguration->Index();
	command.interface.interface_index = fIndex;
	if (ioctl(fRawFD, RAW_COMMAND_GET_INTERFACE_DESCRIPTOR, &command, sizeof(command))
		|| command.interface.status != RAW_STATUS_SUCCESS) {
		memset(&fDescriptor, 0, sizeof(fDescriptor));
	}

	if (fEndpoints) {
		// Delete old endpoints
		for (int32 i = 0; i < fDescriptor.num_endpoints; i++)
			delete fEndpoints[i];
		delete fEndpoints;
	}

	fEndpoints = new BUSBEndpoint *[fDescriptor.num_endpoints];
	for (int32 i = 0; i < fDescriptor.num_endpoints; i++)
		fEndpoints[i] = new BUSBEndpoint(this, i, fRawFD);
}
