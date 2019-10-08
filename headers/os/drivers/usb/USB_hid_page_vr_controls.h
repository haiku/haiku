/*
 * Copyright 2004-2019, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_HID_PAGE_VR_CONTROLS_H
#define _USB_HID_PAGE_VR_CONTROLS_H


/* Reference:
 *		HID Usage Page 0x03: VR Controls
 *		HID Usage Tables Ver. 1.12
 *		http://www.usb.org/developers/devclass_docs/Hut1_12.pdf
 */

// Usage IDs
enum {
	B_HID_UID_VR_BELT = 0x01,
	B_HID_UID_VR_BODY_SUIT,
	B_HID_UID_VR_FLEXOR,
	B_HID_UID_VR_GLOVE,
	B_HID_UID_VR_HEAD_TRACKER,
	B_HID_UID_VR_HEAD_MOUNTED_DISPLAY,
	B_HID_UID_VR_HAND_TRACKER,
	B_HID_UID_VR_OCULOMETER,
	B_HID_UID_VR_VEST,
	B_HID_UID_VR_ANIMATRONIC_DEVICE,

	B_HID_UID_VR_STEREO_ENABLE = 0x20,
	B_HID_UID_VR_DISPLAY_ENABLE,
};


#endif // _USB_HID_PAGE_VR_CONTROLS_H
