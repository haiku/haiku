/*
 * Copyright 2004-2010, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_HID_PAGE_MAGNETIC_STRIPE_READER_H
#define _USB_HID_PAGE_MAGNETIC_STRIPE_READER_H


/* Reference:
 *		HID Usage Page 0x8E: MAGENTIC STRIPE READER
 *		HID Point of Sale Usage Tables Ver. 1.0
 *		http://www.usb.org/developers/devclass_docs/pos1_02.pdf
 */

// Usage IDs
enum {
	B_HID_UID_MSR_DEVICE_READ_ONLY = 0x01,
	
	B_HID_UID_MSR_TRACK_1_LENGTH = 0x11,
	B_HID_UID_MSR_TRACK_2_LENGTH,
	B_HID_UID_MSR_TRACK_3_LENGTH,
	B_HID_UID_MSR_TRACK_JIS_LENGTH,
	
	B_HID_UID_MSR_TRACK_DATA = 0x20,
	B_HID_UID_MSR_TRACK_1_DATA,
	B_HID_UID_MSR_TRACK_2_DATA,
	B_HID_UID_MSR_TRACK_3_DATA,
	B_HID_UID_MSR_TRACK_JIS_DATA	
};


#endif // _USB_HID_PAGE_MAGNETIC_STRIPE_READER_H
