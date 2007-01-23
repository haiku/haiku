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


BUSBEndpoint::BUSBEndpoint(BUSBInterface *interface, uint32 index, int rawFD)
	:	fInterface(interface),
		fIndex(index),
		fRawFD(rawFD)
{
	raw_command command;
	command.endpoint.descriptor = &fDescriptor;
	command.endpoint.config_index = fInterface->Configuration()->Index();
	command.endpoint.interface_index = fInterface->Index();
	command.endpoint.endpoint_index = fIndex;
	if (ioctl(fRawFD, RAW_COMMAND_GET_ENDPOINT_DESCRIPTOR, &command, sizeof(command))
		|| command.config.status != RAW_STATUS_SUCCESS) {
		memset(&fDescriptor, 0, sizeof(fDescriptor));
	}
}


BUSBEndpoint::~BUSBEndpoint()
{
}


uint32
BUSBEndpoint::Index() const
{
	return fIndex;
}


const BUSBInterface *
BUSBEndpoint::Interface() const
{
	return fInterface;
}


const BUSBConfiguration *
BUSBEndpoint::Configuration() const
{
	return fInterface->Configuration();
}


const BUSBDevice *
BUSBEndpoint::Device() const
{
	return fInterface->Device();
}


bool
BUSBEndpoint::IsBulk() const
{
	return (fDescriptor.attributes & USB_ENDPOINT_ATTR_MASK) == USB_ENDPOINT_ATTR_BULK;
}


bool
BUSBEndpoint::IsInterrupt() const
{
	return (fDescriptor.attributes & USB_ENDPOINT_ATTR_MASK) == USB_ENDPOINT_ATTR_INTERRUPT;
}


bool
BUSBEndpoint::IsIsochronous() const
{
	return (fDescriptor.attributes & USB_ENDPOINT_ATTR_MASK) == USB_ENDPOINT_ATTR_ISOCHRONOUS;
}


bool
BUSBEndpoint::IsControl() const
{
	return (fDescriptor.attributes & USB_ENDPOINT_ATTR_MASK) == USB_ENDPOINT_ATTR_CONTROL;
}


bool
BUSBEndpoint::IsInput() const
{
	return (fDescriptor.endpoint_address & USB_ENDPOINT_ADDR_DIR_IN) == USB_ENDPOINT_ADDR_DIR_IN;
}


bool
BUSBEndpoint::IsOutput() const
{
	return (fDescriptor.endpoint_address & USB_ENDPOINT_ADDR_DIR_IN) == USB_ENDPOINT_ADDR_DIR_OUT;
}


uint16
BUSBEndpoint::MaxPacketSize() const
{
	return fDescriptor.max_packet_size;
}


uint8
BUSBEndpoint::Interval() const
{
	return fDescriptor.interval;
}


const usb_endpoint_descriptor *
BUSBEndpoint::Descriptor() const
{
	return &fDescriptor;
}


ssize_t
BUSBEndpoint::ControlTransfer(uint8 requestType, uint8 request, uint16 value,
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


ssize_t
BUSBEndpoint::InterruptTransfer(void *data, size_t length) const
{
	if (length > 0 && data == NULL)
		return B_BAD_VALUE;

	raw_command command;
	command.transfer.interface = fInterface->Index();
	command.transfer.endpoint = fIndex;
	command.transfer.data = data;
	command.transfer.length = length;

	if (ioctl(fRawFD, RAW_COMMAND_INTERRUPT_TRANSFER, &command, sizeof(command))
		|| command.transfer.status != RAW_STATUS_SUCCESS) {
		return B_ERROR;
	}

	return command.transfer.length;
}


ssize_t
BUSBEndpoint::BulkTransfer(void *data, size_t length) const
{
	if (length > 0 && data == NULL)
		return B_BAD_VALUE;

	raw_command command;
	command.transfer.interface = fInterface->Index();
	command.transfer.endpoint = fIndex;
	command.transfer.data = data;
	command.transfer.length = length;

	if (ioctl(fRawFD, RAW_COMMAND_BULK_TRANSFER, &command, sizeof(command))
		|| command.transfer.status != RAW_STATUS_SUCCESS) {
		return B_ERROR;
	}

	return command.transfer.length;
}
