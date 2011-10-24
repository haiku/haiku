/*
 * Copyright 2007-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <USBKit.h>
#include <usb_raw.h>
#include <unistd.h>
#include <string.h>
#include <new>


BUSBConfiguration::BUSBConfiguration(BUSBDevice *device, uint32 index, int rawFD)
	:	fDevice(device),
		fIndex(index),
		fRawFD(rawFD),
		fInterfaces(NULL),
		fConfigurationString(NULL)
{
	usb_raw_command command;
	command.config.descriptor = &fDescriptor;
	command.config.config_index = fIndex;
	if (ioctl(fRawFD, B_USB_RAW_COMMAND_GET_CONFIGURATION_DESCRIPTOR, &command,
		sizeof(command)) || command.config.status != B_USB_RAW_STATUS_SUCCESS)
		memset(&fDescriptor, 0, sizeof(fDescriptor));

	fInterfaces = new(std::nothrow) BUSBInterface *[
		fDescriptor.number_interfaces];
	if (fInterfaces == NULL)
		return;

	for (uint32 i = 0; i < fDescriptor.number_interfaces; i++) {
		fInterfaces[i] = new(std::nothrow) BUSBInterface(this, i,
			B_USB_RAW_ACTIVE_ALTERNATE, fRawFD);
	}
}


BUSBConfiguration::~BUSBConfiguration()
{
	delete[] fConfigurationString;
	if (fInterfaces != NULL) {
		for (int32 i = 0; i < fDescriptor.number_interfaces; i++)
			delete fInterfaces[i];
		delete[] fInterfaces;
	}
}


uint32
BUSBConfiguration::Index() const
{
	return fIndex;
}


const BUSBDevice *
BUSBConfiguration::Device() const
{
	return fDevice;
}


const char *
BUSBConfiguration::ConfigurationString() const
{
	if (fDescriptor.configuration == 0)
		return "";

	if (fConfigurationString)
		return fConfigurationString;

	fConfigurationString = Device()->DecodeStringDescriptor(
		fDescriptor.configuration);
	if (fConfigurationString == NULL)
		return "";

	return fConfigurationString;
}


const usb_configuration_descriptor *
BUSBConfiguration::Descriptor() const
{
	return &fDescriptor;
}


uint32
BUSBConfiguration::CountInterfaces() const
{
	return fDescriptor.number_interfaces;
}


const BUSBInterface *
BUSBConfiguration::InterfaceAt(uint32 index) const
{
	if (index >= fDescriptor.number_interfaces || fInterfaces == NULL)
		return NULL;

	return fInterfaces[index];
}
