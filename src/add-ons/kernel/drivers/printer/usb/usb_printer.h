/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef _USB_PRINTER_H_
#define _USB_PRINTER_H_


#include <lock.h>
#include <USB3.h>


#define PRINTER_INTERFACE_CLASS		0x07
#define PRINTER_INTERFACE_SUBCLASS	0x01

// printer interface types
#define PIT_UNIDIRECTIONAL			0x01
#define PIT_BIDIRECTIONAL			0x02
#define PIT_1284_4_COMPATIBLE		0x03
#define PIT_VENDOR_SPECIFIC			0xff

// class specific requests
// bmRequestType
#define PRINTER_REQUEST				(USB_REQTYPE_INTERFACE_IN \
										| USB_REQTYPE_CLASS)
// bRequest
#define REQUEST_GET_DEVICE_ID		0x00
#define REQUEST_GET_PORT_STATUS		0x01
#define REQUEST_SOFT_RESET			0x02

typedef struct printer_device_s {
	usb_device	device;
	uint32		device_number;
	bool		removed;
	uint32		open_count;
	mutex		lock;
	struct printer_device_s *
				link;

	// device state
	usb_pipe	bulk_in;
	usb_pipe	bulk_out;
	uint8		interface;
	uint8       alternate;
	uint8       alternate_setting;

	// used to store callback information
	sem_id		notify;
	status_t	status;
	size_t		actual_length;

	char		name[32];
} printer_device;

#endif // _USB_PRINTER_H_
