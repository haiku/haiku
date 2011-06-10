/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#include "Prolific.h"

ProlificDevice::ProlificDevice(usb_device device, uint16 vendorID,
	uint16 productID, const char *description)
	:	ACMDevice(device, vendorID, productID, description),
		fIsHX(false)
{
}


status_t
ProlificDevice::AddDevice(const usb_configuration_info *config)
{
	TRACE_FUNCALLS("> ProlificDevice::AddDevice(%08x, %08x)\n", this, config);

	// check for device type.
	// Linux checks for type 0, 1 and HX, but handles 0 and 1 the same.
	// We'll just check for HX then.
	const usb_device_descriptor *deviceDescriptor = NULL;
	deviceDescriptor = gUSBModule->get_device_descriptor(Device());
	if (deviceDescriptor) {
		fIsHX = (deviceDescriptor->device_class != 0x02
			&& deviceDescriptor->max_packet_size_0 == 0x40);
	}

	int32 pipesSet = 0;
	status_t status = ENODEV;
	if (config->interface_count > 0) {
		usb_interface_info *interface = config->interface[0].active;
		for (size_t i = 0; i < interface->endpoint_count; i++) {
			usb_endpoint_info *endpoint = &interface->endpoint[i];
			if (endpoint->descr->attributes == USB_ENDPOINT_ATTR_INTERRUPT) {
				if (endpoint->descr->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN) {
					SetControlPipe(endpoint->handle);
					pipesSet++;
				}
			}
		}

		/* They say that USB-RSAQ1 has 2 interfaces */
		if (config->interface_count >= 2)
			interface = config->interface[1].active;

		for (size_t i = 0; i < interface->endpoint_count; i++) {
			usb_endpoint_info *endpoint = &interface->endpoint[i];
			if (endpoint->descr->attributes == USB_ENDPOINT_ATTR_BULK) {
				if (endpoint->descr->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN)
					SetReadPipe(endpoint->handle);
				else
					SetWritePipe(endpoint->handle);

				if (++pipesSet >= 3)
					break;
			}
		}

		if (pipesSet >= 3)
			status = B_OK;
	}

	TRACE_FUNCRET("< ProlificDevice::AddDevice() returns: 0x%08x\n", status);
	return status;
}


struct request_item {
	bool out;
	uint16 value;
	uint16 index;
};

/* Linux sends all those, and it seems to work */
/* see drivers/usb/serial/pl2303.c */
static struct request_item prolific_reset_common[] = {
	{ false, 0x8484, 0 },
	{ true, 0x0404, 0 },
	{ false, 0x8484, 0 },
	{ false, 0x8383, 0 },
	{ false, 0x8484, 0 },
	{ true, 0x0404, 1 },
	{ false, 0x8484, 0 },
	{ false, 0x8383, 0 },
	{ true, 0x0000, 1 },
	{ true, 0x0001, 0 }
};

static struct request_item prolific_reset_common_hx[] = {
	{ true, 2, 0x44 },
	{ true, 8, 0 },
	{ true, 0, 0 }
};

static struct request_item prolific_reset_common_nhx[] = {
	{ true, 2, 0x24 }
};


status_t
ProlificDevice::SendRequestList(request_item *list, size_t length)
{
	for (size_t i = 0; i < length; i++) {
		char buffer[10];
		size_t bufferLength = 1;
		status_t status = gUSBModule->send_request(Device(),
			USB_REQTYPE_VENDOR | (list[i].out ? USB_REQTYPE_DEVICE_OUT : USB_REQTYPE_DEVICE_IN),
			PROLIFIC_SET_REQUEST,
			list[i].value,
			list[i].index,
			list[i].out ? 0 : bufferLength,
			list[i].out ? NULL : buffer,
			&bufferLength);
		TRACE(" ProlificDevice::SendRequestList(): request[%d]: 0x%08lx\n", i, status);
		if (status != B_OK) {
			TRACE_ALWAYS("sending request list failed:0x%08lx\n", status);
		}
	}

	return B_OK;
}


status_t
ProlificDevice::ResetDevice()
{
	TRACE_FUNCALLS("> ProlificDevice::ResetDevice(%08x)\n", this);

	SendRequestList(prolific_reset_common, SIZEOF(prolific_reset_common));
	if (fIsHX)
		SendRequestList(prolific_reset_common_hx, SIZEOF(prolific_reset_common_hx));
	else
		SendRequestList(prolific_reset_common_nhx, SIZEOF(prolific_reset_common_nhx));

	status_t status = B_OK; /* discard */
	TRACE_FUNCRET("< ProlificDevice::ResetDevice() returns: 0x%08x\n", status);
	return status;
}
