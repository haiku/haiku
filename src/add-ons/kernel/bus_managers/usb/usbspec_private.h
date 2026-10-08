/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */
#ifndef _USBSPEC_PRIVATE_H
#define _USBSPEC_PRIVATE_H


#include <KernelExport.h>
#include <USB3.h>


#define USB_MAX_AREAS					8
#define USB_MAX_FRAGMENT_SIZE			B_PAGE_SIZE * 96
#define USB_MAX_PORT_COUNT				255
#define USB_MAX_DEPTH					5

#define USB_DELAY_BUS_RESET				100000
#define USB_DELAY_HUB_POWER_UP			200000
#define USB_DELAY_PORT_RESET			50000
#define USB_DELAY_PORT_RESET_RECOVERY	50000
#define USB_DELAY_SET_ADDRESS_RETRY		200000
#define USB_DELAY_SET_ADDRESS			10000
#define USB_DELAY_SET_CONFIGURATION		50000
#define USB_DELAY_HUB_EXPLORE			1000000

#define USB_DEBOUNCE_TIMEOUT			1500000
#define USB_DEBOUNCE_CHECK_INTERVAL		25000
#define USB_DEBOUNCE_STABLE_TIME		100000

// For bandwidth calculation
#define	USB_BW_HOST_DELAY					1000
#define	USB_BW_SETUP_LOW_SPEED_PORT_DELAY	333


/*
 * Important data from the USB spec.
 */

struct usb_request_data {
	uint8   RequestType;
	uint8   Request;
	uint16  Value;
	uint16  Index;
	uint16  Length;
} _PACKED;


struct usb_isochronous_data {
	usb_iso_packet_descriptor *packet_descriptors;
	uint32 packet_count;
	uint32 *starting_frame_number;
	uint32 flags;
};


#endif // _USBSPEC_PRIVATE_H
