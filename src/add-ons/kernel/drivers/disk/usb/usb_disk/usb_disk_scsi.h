/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#ifndef _USB_DISK_SCSI_H_
#define _USB_DISK_SCSI_H_

typedef enum {
	SCSI_TEST_UNIT_READY_6 = 0x00,
	SCSI_REQUEST_SENSE_6 = 0x03,
	SCSI_INQUIRY_6 = 0x12,
	SCSI_MODE_SENSE_6 = 0x1a,
	SCSI_START_STOP_UNIT_6 = 0x1b,
	SCSI_SEND_DIAGNOSTIC = 0x1d,
	SCSI_READ_CAPACITY_10 = 0x25,
	SCSI_READ_10 = 0x28,
	SCSI_WRITE_10 = 0x2a,
	SCSI_SYNCHRONIZE_CACHE_10 = 0x35,
	SCSI_READ_12 = 0xA8,
	SCSI_WRITE_12 = 0xAA,
} scsi_operations;

// common command structures
typedef struct scsi_command_6_s {
	uint8	operation;
	uint8	lun;
	uint8	reserved[2];
	uint8	allocation_length;
	uint8	control;
} _PACKED scsi_command_6;

typedef struct scsi_command_10_s {
	uint8	operation;
	uint8	lun_flags;
	uint32	logical_block_address;
	uint8	additional_data;
	uint16	transfer_length;
	uint8	control;
} _PACKED scsi_command_10;

// parameters = returned data
typedef struct scsi_request_sense_6_parameter_s {
	uint8	error_code : 7;
	uint8	valid : 1;
	uint8	segment_number;
	uint8	sense_key : 4;
	uint8	unused_1 : 4;
	uint32	information;
	uint8	additional_sense_length;
	uint32	command_specific_information;
	uint8	additional_sense_code;
	uint8	additional_sense_code_qualifier;
	uint8	field_replacable_unit_code;
	uint8	sense_key_specific[3];
} _PACKED scsi_request_sense_6_parameter;

typedef struct scsi_inquiry_6_parameter_s {
	uint8	peripherial_device_type : 5;
	uint8	peripherial_qualifier : 3;
	uint8	reserved_1 : 7;
	uint8	removable_medium : 1;
	uint8	version;
	uint8	response_data_format : 4;
	uint8	unused_1 : 4;
	uint8	additional_length;
	uint8	unused_2[3];
	char	vendor_identification[8];
	char	product_identification[16];
	char	product_revision_level[4];
} _PACKED scsi_inquiry_6_parameter;

typedef struct scsi_mode_sense_6_parameter_s {
	uint8	data_length;
	uint8	medium_type;
	uint8	device_specific;
	uint8	block_descriptor_length;
	uint8	densitiy;
	uint8	num_blocks[3];
	uint8	reserved;
	uint8	block_length[3];
} _PACKED scsi_mode_sense_6_parameter;

typedef struct scsi_read_capacity_10_parameter_s {
	uint32	last_logical_block_address;
	uint32	logical_block_length;
} _PACKED scsi_read_capacity_10_parameter;

// request sense keys/codes
typedef enum {
	SCSI_SENSE_KEY_NO_SENSE = 0x00,
	SCSI_SENSE_KEY_RECOVERED_ERROR = 0x01,
	SCSI_SENSE_KEY_NOT_READY = 0x02,
	SCSI_SENSE_KEY_MEDIUM_ERROR = 0x03,
	SCSI_SENSE_KEY_HARDWARE_ERROR = 0x04,
	SCSI_SENSE_KEY_ILLEGAL_REQUEST = 0x05,
	SCSI_SENSE_KEY_UNIT_ATTENTION = 0x06,
	SCSI_SENSE_KEY_DATA_PROTECT = 0x07,
	SCSI_SENSE_KEY_ABORTED_COMMAND = 0x0b,
} scsi_sense_key;

// request sense additional sense codes
#define SCSI_ASC_MEDIUM_NOT_PRESENT			0x3a

// mode sense page code/parameter
#define SCSI_MODE_PAGE_DEVICE_CONFIGURATION	0x10
#define SCSI_DEVICE_SPECIFIC_WRITE_PROTECT	0x80

#endif // _USB_DISK_SCSI_H_
