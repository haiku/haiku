/*
 * Copyright 2008-2011, Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */


//!	Driver for USB Human Interface Devices.


#include "Driver.h"
#include "HIDDevice.h"
#include "HIDReport.h"
#include "HIDWriter.h"
#include "ProtocolHandler.h"
#include "QuirkyDevices.h"

#include <usb/USB_hid.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <new>


HIDDevice::HIDDevice(usb_device device, const usb_configuration_info *config,
	size_t interfaceIndex)
	:	fStatus(B_NO_INIT),
		fDevice(device),
		fInterfaceIndex(interfaceIndex),
		fTransferScheduled(0),
		fTransferBufferSize(0),
		fTransferBuffer(NULL),
		fParentCookie(-1),
		fOpenCount(0),
		fRemoved(false),
		fParser(this),
		fProtocolHandlerCount(0),
		fProtocolHandlerList(NULL)
{
	uint8 *reportDescriptor = NULL;
	size_t descriptorLength = 0;

	const usb_device_descriptor *deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	// check for quirky devices first and don't bother in that case
	HIDWriter descriptorWriter;
	bool hasFixedDescriptor = false;
	quirky_init_function quirkyInit = NULL;
	for (int32 i = 0; i < gQuirkyDeviceCount; i++) {
		usb_hid_quirky_device &quirky = gQuirkyDevices[i];
		if (deviceDescriptor->vendor_id == quirky.vendor_id
			&& deviceDescriptor->product_id == quirky.product_id) {

			quirkyInit = quirky.init_function;
			if (quirky.build_descriptor != NULL
				&& quirky.build_descriptor(descriptorWriter) == B_OK) {

				reportDescriptor = (uint8 *)descriptorWriter.Buffer();
				descriptorLength = descriptorWriter.BufferLength();
				hasFixedDescriptor = true;
			}

			break;
		}
	}

	if (!hasFixedDescriptor) {
		// conforming device, read HID and report descriptors
		descriptorLength = sizeof(usb_hid_descriptor);
		usb_hid_descriptor *hidDescriptor
			= (usb_hid_descriptor *)malloc(descriptorLength);
		if (hidDescriptor == NULL) {
			TRACE_ALWAYS("failed to allocate buffer for hid descriptor\n");
			fStatus = B_NO_MEMORY;
			return;
		}

		status_t result = gUSBModule->send_request(device,
			USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_STANDARD,
			USB_REQUEST_GET_DESCRIPTOR,
			B_USB_HID_DESCRIPTOR_HID << 8, interfaceIndex, descriptorLength,
			hidDescriptor, &descriptorLength);

		TRACE("get hid descriptor: result: 0x%08lx; length: %lu\n", result,
			descriptorLength);
		if (result == B_OK) {
			descriptorLength
				= hidDescriptor->descriptor_info[0].descriptor_length;
		} else
			descriptorLength = 256; /* XXX */

		free(hidDescriptor);

		reportDescriptor = (uint8 *)malloc(descriptorLength);
		if (reportDescriptor == NULL) {
			TRACE_ALWAYS("failed to allocate buffer for report descriptor\n");
			fStatus = B_NO_MEMORY;
			return;
		}

		result = gUSBModule->send_request(device,
			USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_STANDARD,
			USB_REQUEST_GET_DESCRIPTOR,
			B_USB_HID_DESCRIPTOR_REPORT << 8, interfaceIndex, descriptorLength,
			reportDescriptor, &descriptorLength);

		TRACE("get report descriptor: result: 0x%08lx; length: %lu\n", result,
			descriptorLength);
		if (result != B_OK) {
			TRACE_ALWAYS("failed tot get report descriptor\n");
			free(reportDescriptor);
			return;
		}
	} else {
		TRACE_ALWAYS("found quirky device, using patched descriptor\n");
	}

#if 1
	// save report descriptor for troubleshooting
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

	status_t result = fParser.ParseReportDescriptor(reportDescriptor,
		descriptorLength);
	if (!hasFixedDescriptor)
		free(reportDescriptor);

	if (result != B_OK) {
		TRACE_ALWAYS("parsing the report descriptor failed\n");
		fStatus = result;
		return;
	}

#if 0
	for (uint32 i = 0; i < fParser.CountReports(HID_REPORT_TYPE_ANY); i++)
		fParser.ReportAt(HID_REPORT_TYPE_ANY, i)->PrintToStream();
#endif

	// find the interrupt in pipe
	usb_interface_info *interface = config->interface[interfaceIndex].active;
	for (size_t i = 0; i < interface->endpoint_count; i++) {
		usb_endpoint_descriptor *descriptor = interface->endpoint[i].descr;
		if ((descriptor->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN)
			&& (descriptor->attributes & USB_ENDPOINT_ATTR_MASK)
				== USB_ENDPOINT_ATTR_INTERRUPT) {
			fInterruptPipe = interface->endpoint[i].handle;
			break;
		}
	}

	if (fInterruptPipe == 0) {
		TRACE_ALWAYS("didn't find a suitable interrupt pipe\n");
		return;
	}

	fTransferBufferSize = fParser.MaxReportSize();
	if (fTransferBufferSize == 0) {
		TRACE_ALWAYS("report claims a report size of 0\n");
		return;
	}

	fTransferBuffer = (uint8 *)malloc(fTransferBufferSize);
	if (fTransferBuffer == NULL) {
		TRACE_ALWAYS("failed to allocate transfer buffer\n");
		fStatus = B_NO_MEMORY;
		return;
	}

	if (quirkyInit != NULL) {
		fStatus = quirkyInit(device, config, interfaceIndex);
		if (fStatus != B_OK)
			return;
	}

	ProtocolHandler::AddHandlers(*this, fProtocolHandlerList,
		fProtocolHandlerCount);
	fStatus = B_OK;
}


HIDDevice::~HIDDevice()
{
	ProtocolHandler *handler = fProtocolHandlerList;
	while (handler != NULL) {
		ProtocolHandler *next = handler->NextHandler();
		delete handler;
		handler = next;
	}

	free(fTransferBuffer);
}


void
HIDDevice::SetParentCookie(int32 cookie)
{
	fParentCookie = cookie;
}


status_t
HIDDevice::Open(ProtocolHandler *handler, uint32 flags)
{
	atomic_add(&fOpenCount, 1);
	return B_OK;
}


status_t
HIDDevice::Close(ProtocolHandler *handler)
{
	atomic_add(&fOpenCount, -1);
	return B_OK;
}


void
HIDDevice::Removed()
{
	fRemoved = true;
	gUSBModule->cancel_queued_transfers(fInterruptPipe);
}


status_t
HIDDevice::MaybeScheduleTransfer()
{
	if (fRemoved)
		return B_ERROR;

	if (atomic_set(&fTransferScheduled, 1) != 0) {
		// someone else already caused a transfer to be scheduled
		return B_OK;
	}

	TRACE("scheduling interrupt transfer of %lu bytes\n", fTransferBufferSize);
	status_t result = gUSBModule->queue_interrupt(fInterruptPipe,
		fTransferBuffer, fTransferBufferSize, _TransferCallback, this);
	if (result != B_OK) {
		TRACE_ALWAYS("failed to schedule interrupt transfer 0x%08lx\n", result);
		return result;
	}

	return B_OK;
}


status_t
HIDDevice::SendReport(HIDReport *report)
{
	size_t actualLength;
	return gUSBModule->send_request(fDevice,
		USB_REQTYPE_INTERFACE_OUT | USB_REQTYPE_CLASS,
		B_USB_REQUEST_HID_SET_REPORT, 0x200 | report->ID(), fInterfaceIndex,
		report->ReportSize(), report->CurrentReport(), &actualLength);
}


ProtocolHandler *
HIDDevice::ProtocolHandlerAt(uint32 index) const
{
	ProtocolHandler *handler = fProtocolHandlerList;
	while (handler != NULL) {
		if (index == 0)
			return handler;

		handler = handler->NextHandler();
		index--;
	}

	return NULL;
}


void
HIDDevice::_TransferCallback(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	HIDDevice *device = (HIDDevice *)cookie;
	if (status == B_DEV_STALLED && !device->fRemoved) {
		// try clearing stalls right away, the report listeners will resubmit
		gUSBModule->clear_feature(device->fInterruptPipe,
			USB_FEATURE_ENDPOINT_HALT);
	}

	atomic_set(&device->fTransferScheduled, 0);
	device->fParser.SetReport(status, device->fTransferBuffer, actualLength);
}
