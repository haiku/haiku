/*
 * Copyright 2026, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef USB_MISC_H
#define USB_MISC_H


#define USB_MISCELLANEOUS_CLASS	0xEF

// See https://www.usb.org/defined-class-codes for class, subclass and protocol registry

// Miscellaneous Interface Subclasses
enum {
	B_USB_MISC_PDA_SYNC_SUBCLASS = 0x01,
	B_USB_MISC_MULTIPLEXED_SUBCLASS = 0x02,
	B_USB_MISC_CABLE_BASED_SUBCLASS = 0x03,
	B_USB_MISC_RNDIS_SUBCLASS = 0x04,
	B_USB_MISC_USB_VISION_SUBCLASS = 0x05,
	B_USB_MISC_STEP_SUBCLASS = 0x06,
	B_USB_MISC_DVB_CI_SUBCLASS = 0x07,
	B_USB_MISC_SECURE_FIRMWARE_SUBCLASS = 0x08,
	B_USB_MISC_OBMF_SUBCLASS = 0x09,
};

// PDA sync subclass protocols
enum {
	B_USB_ACTIVE_SYNC_PROTOCOL = 0x01,
	B_USB_PALM_SYNC_PROTOCOL = 0x02,
};

// RNDIS subclass protocols
enum {
	B_USB_RNDIS_ETHERNET_PROTOCOL = 0x01,
	B_USB_RNDIS_WIFI_PROTOCOL = 0x02,
	B_USB_RNDIS_WIMAX_PROTOCOL = 0x03,
	B_USB_RNDIS_WWAN_PROTOCOL = 0x04,
	B_USB_RNDIS_RAW_IPV4_PROTOCOL = 0x05,
	B_USB_RNDIS_RAW_IPV6_PROTOCOL = 0x06,
	B_USB_RNDIS_GPRS_PROTOCOL = 0x07,
};

#endif /* !USB_MISC_H */
