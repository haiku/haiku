/*
 * Copyright 2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef USB_HID_QUIRKY_DEVICES
#define USB_HID_QUIRKY_DEVICES

#include "Driver.h"

class HIDWriter;

typedef status_t (*quirky_init_function)(usb_device device,
	const usb_configuration_info *config, size_t interfaceIndex);
typedef status_t (*quirky_build_descriptor)(HIDWriter &writer);

struct usb_hid_quirky_device {
	uint16					vendor_id;
	uint16					product_id;
	quirky_init_function	init_function;
	quirky_build_descriptor	build_descriptor;
};

extern usb_hid_quirky_device gQuirkyDevices[];
extern int32 gQuirkyDeviceCount;

#endif // USB_HID_QUIRKY_DEVICES
