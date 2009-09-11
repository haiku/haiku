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
#define PRINTER_REQUEST				0xA1
// bRequest
#define REQUEST_GET_STATUS_ID		0x00
#define REQUEST_GET_PORT_STATUS		0x01
#define REQUEST_SOFT_RESET			0x02

#if 0
#define CBW_SIGNATURE				0x43425355
#define CBW_DATA_OUTPUT				0x00
#define CBW_DATA_INPUT				0x80

#define CSW_SIGNATURE				0x53425355
#define CSW_STATUS_COMMAND_PASSED	0x00
#define CSW_STATUS_COMMAND_FAILED	0x01
#define CSW_STATUS_PHASE_ERROR		0x02

#endif

// TODO remove?
#define SYNC_SUPPORT_RELOAD			5

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
	uint32		current_tag;
	uint8		sync_support;

	// used to store callback information
	sem_id		notify;
	status_t	status;
	size_t		actual_length;

	char		name[32];
	uint32		block_size;
} printer_device;

#if 0
// represents a logical unit on the pointed to device - this gets published
struct device_lun_s {
	disk_device *device;
	char		name[32];
	uint8		logical_unit_number;
	bool		should_sync;

	// device information through read capacity/inquiry
	bool		media_present;
	bool		media_changed;
	uint32		block_count;
	uint32		block_size;
	uint8		device_type;
	bool		removable;
	bool		write_protected;
};


typedef struct command_block_wrapper_s {
	uint32		signature;
	uint32		tag;
	uint32		data_transfer_length;
	uint8		flags;
	uint8		lun;
	uint8		command_block_length;
	uint8		command_block[16];
} _PACKED command_block_wrapper;


typedef struct command_status_wrapper_s {
	uint32		signature;
	uint32		tag;
	uint32		data_residue;
	uint8		status;
} _PACKED command_status_wrapper;
#endif

#endif // _USB_PRINTER_H_
