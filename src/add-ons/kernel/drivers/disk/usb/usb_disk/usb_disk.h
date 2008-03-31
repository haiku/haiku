/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#ifndef _USB_DISK_H_
#define _USB_DISK_H_

#include <lock.h>
#include <USB3.h>

#define REQUEST_MASS_STORAGE_RESET	0xff
#define REQUEST_GET_MAX_LUN			0xfe

#define CBW_SIGNATURE				0x43425355
#define CBW_DATA_OUTPUT				0x00
#define CBW_DATA_INPUT				0x80

#define CSW_SIGNATURE				0x53425355
#define CSW_STATUS_COMMAND_PASSED	0x00
#define CSW_STATUS_COMMAND_FAILED	0x01
#define CSW_STATUS_PHASE_ERROR		0x02

typedef struct disk_device_s {
	usb_device	device;
	bool		removed;
	uint32		open_count;
	benaphore	lock;
	char		name[32];
	void *		link;

	// device state
	usb_pipe	bulk_in;
	usb_pipe	bulk_out;
	uint8		interface;
	uint32		current_tag;
	bool		should_sync;
	uint8		lun;

	// device information through read capacity/inquiry
	uint32		block_count;
	uint32		block_size;
	uint8		device_type;
	bool		removable;

	// used to store callback information
	sem_id		notify;
	status_t	status;
	size_t		actual_length;
} disk_device;


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

#endif // _USB_DISK_H_
