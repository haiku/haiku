/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <USBKit.h>
#include <usb_raw.h>
#include <unistd.h>
#include <string.h>


BUSBConfiguration::BUSBConfiguration(BUSBDevice *device, uint32 index, int rawFD)
	:	fDevice(device),
		fIndex(index),
		fRawFD(rawFD),
		fInterfaces(NULL),
		fConfigurationString(NULL)
{
	raw_command command;
	command.config.descriptor = &fDescriptor;
	command.config.config_index = fIndex;
	if (ioctl(fRawFD, RAW_COMMAND_GET_CONFIGURATION_DESCRIPTOR, &command, sizeof(command))
		|| command.config.status != RAW_STATUS_SUCCESS) {
		memset(&fDescriptor, 0, sizeof(fDescriptor));
	}

	fInterfaces = new BUSBInterface *[fDescriptor.number_interfaces];
	for (uint32 i = 0; i < fDescriptor.number_interfaces; i++)
		fInterfaces[i] = new BUSBInterface(this, i, fRawFD);
}


BUSBConfiguration::~BUSBConfiguration()
{
	delete[] fConfigurationString;
	for (int32 i = 0; i < fDescriptor.number_interfaces; i++)
		delete fInterfaces[i];
	delete[] fInterfaces;
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

	fConfigurationString = Device()->DecodeStringDescriptor(fDescriptor.configuration);
	if (!fConfigurationString) {
		fConfigurationString = new char[1];
		fConfigurationString[0] = 0;
	}

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
	if (index >= fDescriptor.number_interfaces)
		return NULL;

	return fInterfaces[index];
}
