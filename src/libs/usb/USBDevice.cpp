/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <USBKit.h>
#include <usb_raw.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>


BUSBDevice::BUSBDevice(const char *path)
	:	fPath(NULL),
		fRawFD(-1),
		fConfigurations(NULL),
		fActiveConfiguration(0),
		fManufacturerString(NULL),
		fProductString(NULL),
		fSerialNumberString(NULL)
{
	memset(&fDescriptor, 0, sizeof(fDescriptor));

	if (path)
		SetTo(path);
}


BUSBDevice::~BUSBDevice()
{
	Unset();
}


status_t
BUSBDevice::InitCheck()
{
	return (fRawFD >= 0 ? B_OK : B_ERROR);
}


status_t
BUSBDevice::SetTo(const char *path)
{
	if (!path)
		return B_BAD_VALUE;

	fPath = strdup(path);
	fRawFD = open(path, O_RDWR);
	if (fRawFD < 0) {
		Unset();
		return B_ERROR;
	}

	raw_command command;
	if (ioctl(fRawFD, RAW_COMMAND_GET_VERSION, &command, sizeof(command))
		|| command.version.status != 0x0015) {
		Unset();
		return B_ERROR;
	}

	command.device.descriptor = &fDescriptor;
	if (ioctl(fRawFD, RAW_COMMAND_GET_DEVICE_DESCRIPTOR, &command, sizeof(command))
		|| command.device.status != RAW_STATUS_SUCCESS) {
		Unset();
		return B_ERROR;
	}

	fConfigurations = new BUSBConfiguration *[fDescriptor.num_configurations];
	for (uint32 i = 0; i < fDescriptor.num_configurations; i++)
		fConfigurations[i] = new BUSBConfiguration(this, i, fRawFD);

	return B_OK;
}


void
BUSBDevice::Unset()
{
	if (fRawFD >= 0)
		close(fRawFD);
	fRawFD = -1;

	free(fPath);
	fPath = NULL;

	delete[] fManufacturerString;
	delete[] fProductString;
	delete[] fSerialNumberString;
	fManufacturerString = fProductString = fSerialNumberString = NULL;

	for (int32 i = 0; i < fDescriptor.num_configurations; i++)
		delete fConfigurations[i];

	delete[] fConfigurations;
	memset(&fDescriptor, 0, sizeof(fDescriptor));
}


const char *
BUSBDevice::Location() const
{
	if (!fPath || strlen(fPath) < 12)
		return NULL;

	return &fPath[12];
}


bool
BUSBDevice::IsHub() const
{
	return fDescriptor.device_class == 0x09;
}


uint16
BUSBDevice::USBVersion() const
{
	return fDescriptor.usb_version;
}


uint8
BUSBDevice::Class() const
{
	return fDescriptor.device_class;
}


uint8
BUSBDevice::Subclass() const
{
	return fDescriptor.device_subclass;
}


uint8
BUSBDevice::Protocol() const
{
	return fDescriptor.device_protocol;
}


uint8
BUSBDevice::MaxEndpoint0PacketSize() const
{
	return fDescriptor.max_packet_size_0;
}


uint16
BUSBDevice::VendorID() const
{
	return fDescriptor.vendor_id;
}


uint16
BUSBDevice::ProductID() const
{
	return fDescriptor.product_id;
}


uint16
BUSBDevice::Version() const
{
	return fDescriptor.device_version;
}


const char *
BUSBDevice::ManufacturerString() const
{
	if (fDescriptor.manufacturer == 0)
		return "";

	if (fManufacturerString)
		return fManufacturerString;

	fManufacturerString = DecodeStringDescriptor(fDescriptor.manufacturer);
	if (!fManufacturerString) {
		fManufacturerString = new char[1];
		fManufacturerString[0] = 0;
	}

	return fManufacturerString;
}


const char *
BUSBDevice::ProductString() const
{
	if (fDescriptor.product == 0)
		return "";

	if (fProductString)
		return fProductString;

	fProductString = DecodeStringDescriptor(fDescriptor.product);
	if (!fProductString) {
		fProductString = new char[1];
		fProductString[0] = 0;
	}

	return fProductString;
}


const char *
BUSBDevice::SerialNumberString() const
{
	if (fDescriptor.serial_number == 0)
		return "";

	if (fSerialNumberString)
		return fSerialNumberString;

	fSerialNumberString = DecodeStringDescriptor(fDescriptor.serial_number);
	if (!fSerialNumberString) {
		fSerialNumberString = new char[1];
		fSerialNumberString[0] = 0;
	}

	return fSerialNumberString;
}


const usb_device_descriptor *
BUSBDevice::Descriptor() const
{
	return &fDescriptor;
}


size_t
BUSBDevice::GetStringDescriptor(uint32 index,
	usb_string_descriptor *descriptor, size_t length) const
{
	if (!descriptor)
		return B_BAD_VALUE;

	raw_command command;
	command.string.descriptor = descriptor;
	command.string.string_index = index;
	command.string.length = length;

	if (ioctl(fRawFD, RAW_COMMAND_GET_STRING_DESCRIPTOR, &command, sizeof(command))
		|| command.string.status != RAW_STATUS_SUCCESS) {
		return 0;
	}

	return command.string.length;
}


char *
BUSBDevice::DecodeStringDescriptor(uint32 index) const
{
	char buffer[300];
	usb_string_descriptor *stringDescriptor;
	stringDescriptor = (usb_string_descriptor *)&buffer;

	size_t stringLength = GetStringDescriptor(index, stringDescriptor,
		sizeof(buffer) - sizeof(usb_string_descriptor));

	if (stringLength < 3)
		return NULL;

	// pseudo convert unicode string
	stringLength = (stringLength - 2) / 2;
	char *result = new char[stringLength + 1];
	for (size_t i = 0; i < stringLength; i++)
		result[i] = stringDescriptor->string[i * 2];
	result[stringLength] = 0;
	return result;
}


size_t
BUSBDevice::GetDescriptor(uint8 type, uint8 index, uint16 languageID,
	void *data, size_t length) const
{
	if (length > 0 && data == NULL)
		return B_BAD_VALUE;

	raw_command command;
	command.descriptor.type = type;
	command.descriptor.index = index;
	command.descriptor.language_id = languageID;
	command.descriptor.data = data;
	command.descriptor.length = length;

	if (ioctl(fRawFD, RAW_COMMAND_GET_DESCRIPTOR, &command, sizeof(command))
		|| command.descriptor.status != RAW_STATUS_SUCCESS) {
		return 0;
	}

	return command.descriptor.length;
}


uint32
BUSBDevice::CountConfigurations() const
{
	return fDescriptor.num_configurations;
}


const BUSBConfiguration *
BUSBDevice::ConfigurationAt(uint32 index) const
{
	if (index >= fDescriptor.num_configurations)
		return NULL;

	return fConfigurations[index];
}


const BUSBConfiguration *
BUSBDevice::ActiveConfiguration() const
{
	return fConfigurations[fActiveConfiguration];
}


status_t
BUSBDevice::SetConfiguration(const BUSBConfiguration *configuration)
{
	if (!configuration || configuration->Index() >= fDescriptor.num_configurations)
		return B_BAD_VALUE;

	raw_command command;
	command.config.config_index = configuration->Index();

	if (ioctl(fRawFD, RAW_COMMAND_SET_CONFIGURATION, &command, sizeof(command))
		|| command.config.status != RAW_STATUS_SUCCESS) {
		return B_ERROR;
	}

	fActiveConfiguration = configuration->Index();
	return B_OK;
}


ssize_t
BUSBDevice::ControlTransfer(uint8 requestType, uint8 request, uint16 value,
	uint16 index, uint16 length, void *data) const
{
	if (length > 0 && data == NULL)
		return B_BAD_VALUE;

	raw_command command;
	command.control.request_type = requestType;
	command.control.request = request;
	command.control.value = value;
	command.control.index = index;
	command.control.length = length;
	command.control.data = data;

	if (ioctl(fRawFD, RAW_COMMAND_CONTROL_TRANSFER, &command, sizeof(command))
		|| command.control.status != RAW_STATUS_SUCCESS) {
		return B_ERROR;
	}

	return command.control.length;
}
