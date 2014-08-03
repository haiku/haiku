/*
 * Copyright 2014, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_MSC_H
#define _USB_MSC_H


// (Partial) USB Class Definitions for Mass Storage Devices (MSC), version 1.0
// Reference: http://www.usb.org/developers/devclass_docs/usbmassbulk_10.pdf


#define USB_MASS_STORAGE_DEVICE_CLASS			0x08

#define USB_MASSBULK_CBW_SIGNATURE				0x43425355
#define USB_MASSBULK_CBW_DATA_OUTPUT			0x00
#define USB_MASSBULK_CBW_DATA_INPUT				0x80

#define USB_MASSBULK_CSW_SIGNATURE				0x53425355
#define USB_MASSBULK_CSW_STATUS_COMMAND_PASSED	0x00
#define USB_MASSBULK_CSW_STATUS_COMMAND_FAILED	0x01
#define USB_MASSBULK_CSW_STATUS_PHASE_ERROR		0x02

#define USB_MASSBULK_REQUEST_MASS_STORAGE_RESET	0xff
#define USB_MASSBULK_REQUEST_GET_MAX_LUN		0xfe


typedef struct {
	uint32		signature;
	uint32		tag;
	uint32		data_transfer_length;
	uint8		flags;
	uint8		lun;
	uint8		command_block_length;
	uint8		command_block[16];
} _PACKED usb_massbulk_command_block_wrapper;


typedef struct {
	uint32		signature;
	uint32		tag;
	uint32		data_residue;
	uint8		status;
} _PACKED usb_massbulk_command_status_wrapper;


#endif

