/*
 * Copyright 2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef USB_HID_QUIRKY_DEVICES
#define USB_HID_QUIRKY_DEVICES

static uint8 sDescriptorExample[] = { 0x00, 0x00 };

struct usb_hid_quirky_device {
	uint16			vendor_id;
	uint16			product_id;
	size_t			descriptor_length;
	const uint8 *	fixed_descriptor;
};

#define ADD_QUIRKY_DEVICE(vendorID, productID, descriptor) \
	{ vendorID, productID, sizeof(descriptor), descriptor }

static usb_hid_quirky_device sQuirkyDevices[] = {
	ADD_QUIRKY_DEVICE(0xffff, 0xffff, sDescriptorExample)
};

static int32 sQuirkyDeviceCount
	= sizeof(sQuirkyDevices) / sizeof(sQuirkyDevices[0]);

#endif // USB_HID_QUIRKY_DEVICES
