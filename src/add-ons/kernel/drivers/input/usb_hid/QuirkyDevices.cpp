/*
 * Copyright 2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */


#include "QuirkyDevices.h"

#include <string.h>

#include <usb/USB_hid.h>


status_t
sixaxis_init(usb_device device, const usb_configuration_info *config,
	size_t interfaceIndex)
{
	TRACE_ALWAYS("found SIXAXIS controller, putting it in operational mode\n");
	uint8 dummy[18];
	status_t result = gUSBModule->send_request(device, USB_REQTYPE_INTERFACE_IN
			| USB_REQTYPE_CLASS, B_USB_REQUEST_HID_GET_REPORT, 0x03f2 /* ? */,
		interfaceIndex, sizeof(dummy), dummy, NULL);
	if (result != B_OK) {
		TRACE_ALWAYS("failed to set operational mode: %s\n", strerror(result));
	}

	return result;
}


#define ADD_QUIRKY_DEVICE(vendorID, productID, descriptor, init) \
	{ vendorID, productID, sizeof(descriptor), descriptor, init }

usb_hid_quirky_device gQuirkyDevices[] = {
	// Sony SIXAXIS controller (PS3) needs a GET_REPORT to become operational
	ADD_QUIRKY_DEVICE(0x054c, 0x0268, NULL, sixaxis_init),
};

int32 gQuirkyDeviceCount
	= sizeof(gQuirkyDevices) / sizeof(gQuirkyDevices[0]);
