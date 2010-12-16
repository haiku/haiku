/*
 * Copyright 2004-2010, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_HID_PAGE_USB_MONITOR_H
#define _USB_HID_PAGE_USB_MONITOR_H


/* Reference:
 *		HID Usage Page 0x80: USB MONITOR
 *		USB Monitor Control Class Specification, Rev. 1.0
 *		http://www.usb.org/developers/devclass_docs/usbmon10.pdf
 */

// Usage IDs
enum {
	B_HID_UID_MON_MONITOR_CONTROL = 0x01,
	B_HID_UID_MON_EDID_INFORMATION,
	B_HID_UID_MON_VDIF_INFORMATION,
	B_HID_UID_MON_VESA_VERSION
};


#endif // _USB_HID_PAGE_USB_MONITOR_H
