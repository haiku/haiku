/*
	Driver for USB Human Interface Devices.
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#include "Driver.h"
#include "HIDDevice.h"
#include <usb/USB_hid.h>
#include <stdio.h>
#include <string.h>
#include <new>

// includes for the different device types
#include "KeyboardDevice.h"
#include "MouseDevice.h"


HIDDevice::HIDDevice(usb_device device, usb_pipe interruptPipe,
	size_t interfaceIndex, report_insn *instructions, size_t instructionCount,
	size_t totalReportSize, size_t ringBufferSize)
	:	fStatus(B_NO_INIT),
		fDevice(device),
		fInterruptPipe(interruptPipe),
		fInterfaceIndex(interfaceIndex),
		fInstructions(instructions),
		fInstructionCount(instructionCount),
		fTotalReportSize(totalReportSize),
		fTransferUnprocessed(false),
		fTransferStatus(B_ERROR),
		fTransferActualLength(0),
		fTransferBuffer(NULL),
		fTransferNotifySem(-1),
		fName(NULL),
		fParentCookie(-1),
		fOpen(false),
		fRemoved(false),
		fRingBuffer(NULL)
{
	if (ringBufferSize > 0)
		fRingBuffer = create_ring_buffer(ringBufferSize);

	fTransferBuffer = (uint8 *)malloc(fTotalReportSize);
	if (fTransferBuffer == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	fStatus = B_OK;
}


HIDDevice::~HIDDevice()
{
	if (fRingBuffer) {
		delete_ring_buffer(fRingBuffer);
		fRingBuffer = NULL;
	}

	if (fTransferNotifySem >= 0)
		delete_sem(fTransferNotifySem);

	free(fTransferBuffer);
	free(fName);
}


HIDDevice *
HIDDevice::MakeHIDDevice(usb_device device,
	const usb_configuration_info *config, size_t interfaceIndex)
{
	// read HID descriptor
	size_t descriptorLength = sizeof(usb_hid_descriptor);
	usb_hid_descriptor *hidDescriptor = (usb_hid_descriptor *)malloc(descriptorLength);
	if (hidDescriptor == NULL)
		return NULL;

	status_t result = gUSBModule->send_request(device,
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_STANDARD,
		USB_REQUEST_GET_DESCRIPTOR,
		USB_HID_DESCRIPTOR_HID << 8, interfaceIndex, descriptorLength,
		hidDescriptor, &descriptorLength);

	TRACE("get_hid_desc: result: 0x%08lx; length: %lu\n", result, descriptorLength);
	if (result == B_OK)
		descriptorLength = hidDescriptor->descriptor_info[0].descriptor_length;
	else
		descriptorLength = 256; /* XXX */
	free(hidDescriptor);

	uint8 *reportDescriptor = (uint8 *)malloc(descriptorLength);
	if (reportDescriptor == NULL)
		return NULL;

	result = gUSBModule->send_request(device,
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_STANDARD,
		USB_REQUEST_GET_DESCRIPTOR,
		USB_HID_DESCRIPTOR_REPORT << 8, interfaceIndex, descriptorLength,
		reportDescriptor, &descriptorLength);

	TRACE("get_hid_rep_desc: result: 0x%08lx; length: %lu\n", result, descriptorLength);
	if (result != B_OK) {
		free(reportDescriptor);
		return NULL;
	}

#if 1
	// save report descriptor for troubleshooting
	const usb_device_descriptor *deviceDescriptor
		= gUSBModule->get_device_descriptor(device);
	char outputFile[128];
	sprintf(outputFile, "/tmp/usb_hid_report_descriptor_%04x_%04x_%lu.bin",
		deviceDescriptor->vendor_id, deviceDescriptor->product_id,
		interfaceIndex);
	int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd >= 0) {
		write(fd, reportDescriptor, descriptorLength);
		close(fd);
	}
#endif

	// decompose report descriptor
	size_t itemCount = descriptorLength;
	decomp_item *items = (decomp_item *)malloc(sizeof(decomp_item) * itemCount);
	if (items == NULL) {
		free(reportDescriptor);
		return NULL;
	}

	decompose_report_descriptor(reportDescriptor, descriptorLength, items,
		&itemCount);
	uint32 deviceType = *(uint32 *)reportDescriptor;
	free(reportDescriptor);

	// parse report descriptor
	size_t instructionCount = itemCount;
	report_insn *instructions
		= (report_insn *)malloc(sizeof(report_insn) * instructionCount);
	if (instructions == NULL) {
		free(items);
		return NULL;
	}

	int firstReportID = 0;
	size_t totalReportSize = 0;
	parse_report_descriptor(items, itemCount, instructions,
		&instructionCount, &totalReportSize, &firstReportID);
	free(items);

	report_insn *finalInstructions = (report_insn *)realloc(instructions,
		sizeof(report_insn) * instructionCount);
	if (finalInstructions == NULL) {
		free(instructions);
		return NULL;
	}

	TRACE("%lu items, %lu instructions, %lu bytes\n", itemCount,
		instructionCount, totalReportSize);

	// find the interrupt in pipe
	usb_pipe interruptPipe = 0;
	usb_interface_info *interface = config->interface[interfaceIndex].active;
	for (size_t i = 0; i < interface->endpoint_count; i++) {
		usb_endpoint_descriptor *descriptor = interface->endpoint[i].descr;
		if ((descriptor->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN)
			&& (descriptor->attributes & USB_ENDPOINT_ATTR_MASK) == USB_ENDPOINT_ATTR_INTERRUPT) {
			interruptPipe = interface->endpoint[i].handle;
			break;
		}
	}

	if (interruptPipe == 0) {
		TRACE_ALWAYS("didn't find a suitable interrupt pipe\n");
		free(finalInstructions);
		return NULL;
	}

	// determine device type and create the device object
	HIDDevice *hidDevice = NULL;
	if (deviceType == USB_HID_DEVICE_TYPE_KEYBOARD) {
		hidDevice = new(std::nothrow) KeyboardDevice(device, interruptPipe,
			interfaceIndex, finalInstructions, instructionCount,
			totalReportSize);
	} else if (deviceType == USB_HID_DEVICE_TYPE_MOUSE) {
		hidDevice = new(std::nothrow) MouseDevice(device, interruptPipe,
			interfaceIndex, finalInstructions, instructionCount,
			totalReportSize);
	} else
		TRACE_ALWAYS("unsupported device type 0x%08lx\n", deviceType);

	if (hidDevice == NULL)
		free(finalInstructions);
	return hidDevice;
}


void
HIDDevice::SetBaseName(const char *baseName)
{
	// As devices can be un- and replugged at will, we cannot simply rely on
	// a device count. If there is just one keyboard, this does not mean that
	// it uses the 0 name. There might have been two keyboards and the one
	// using 0 might have been unplugged. So we just generate names until we
	// find one that is not currently in use.
	int32 index = 0;
	char nameBuffer[128];
	while (true) {
		sprintf(nameBuffer, "%s%ld", baseName, index++);
		if (gDeviceList->FindDevice(nameBuffer) == NULL) {
			// this name is still free, use it
			free(fName);
			fName = strdup(nameBuffer);
			return;
		}
	}
}


void
HIDDevice::SetParentCookie(int32 cookie)
{
	fParentCookie = cookie;
}


status_t
HIDDevice::Open(uint32 flags)
{
	if (fOpen)
		return B_BUSY;

	if (fTransferNotifySem < 0) {
		fTransferNotifySem = create_sem(0, "hid device transfer notify sem");
		if (fTransferNotifySem < 0)
			return (status_t)fTransferNotifySem;
	}

	fOpen = true;

	return B_OK;
}


status_t
HIDDevice::Close()
{
	fOpen = false;
	// make threads waiting for a transfer bail out
	if (fTransferNotifySem >= 0) {
		gUSBModule->cancel_queued_transfers(fInterruptPipe);
		delete_sem(fTransferNotifySem);
		fTransferNotifySem = -1;
		_SetTransferProcessed();
	}
	return B_OK;
}


status_t
HIDDevice::Free()
{
	return B_OK;
}


status_t
HIDDevice::Read(uint8 *buffer, size_t *numBytes)
{
	TRACE_ALWAYS("read on hid device\n");
	*numBytes = 0;
	return B_ERROR;
}


status_t
HIDDevice::Write(const uint8 *buffer, size_t *numBytes)
{
	TRACE_ALWAYS("write on hid device\n");
	*numBytes = 0;
	return B_ERROR;
}


status_t
HIDDevice::Control(uint32 op, void *buffer, size_t length)
{
	TRACE_ALWAYS("control on base class\n");
	return B_ERROR;
}


void
HIDDevice::Removed()
{
	fRemoved = true;
	gUSBModule->cancel_queued_transfers(fInterruptPipe);
}


void
HIDDevice::_SetTransferProcessed()
{
	fTransferUnprocessed = false;
}


bool
HIDDevice::_IsTransferUnprocessed()
{
	return fTransferUnprocessed;
}


status_t
HIDDevice::_ScheduleTransfer()
{
	if (fTransferNotifySem < 0)
		return B_ERROR;
	if (fTransferUnprocessed)
		return B_BUSY;
	if (fRemoved)
		return B_ERROR;

	status_t result = gUSBModule->queue_interrupt(fInterruptPipe,
		fTransferBuffer, fTotalReportSize, _TransferCallback, this);
	if (result < B_OK) {
		TRACE_ALWAYS("failed to schedule interrupt transfer 0x%08lx\n", result);
		return result;
	}

	fTransferUnprocessed = true;
	return B_OK;
}


int32
HIDDevice::_RingBufferReadable()
{
	return ring_buffer_readable(fRingBuffer);
}


status_t
HIDDevice::_RingBufferRead(void *buffer, size_t length)
{
	ring_buffer_user_read(fRingBuffer, (uint8 *)buffer, length);
	return B_OK;
}


status_t
HIDDevice::_RingBufferWrite(const void *buffer, size_t length)
{
	ring_buffer_write(fRingBuffer, (const uint8 *)buffer, length);
	return B_OK;
}


void
HIDDevice::_TransferCallback(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	HIDDevice *device = (HIDDevice *)cookie;
	device->fTransferStatus = status;
	device->fTransferActualLength = actualLength;
	release_sem_etc(device->fTransferNotifySem, 1, B_DO_NOT_RESCHEDULE);
}
