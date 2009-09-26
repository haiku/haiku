/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef VMWARE_TYPES_H
#define VMWARE_TYPES_H

#define VMWARE_PORT_MAGIC					0x564d5868
#define VMWARE_PORT_NUMBER					0x5658

#define VMWARE_VERSION_ID					0x3442554a
#define VMWARE_ERROR						0xffff

#define VMWARE_VALUE_DISABLE				0x000000f5
#define VMWARE_VALUE_READ_ID				0x45414552
#define VMWARE_VALUE_REQUEST_ABSOLUTE		0x53424152

#define VMWARE_COMMAND_POINTER_DATA		39
#define VMWARE_COMMAND_POINTER_STATUS		40
#define VMWARE_COMMAND_POINTER_COMMAND		41


struct command_s {
	uint32	magic;
	uint32	value;
	uint32	command;
	uint32	port;
} _PACKED;


struct data_s {
	uint16	buttons;
	uint16	flags;
	int32	x; // signed when relative
	int32	y; // signed when relative
	int32	z; // always signed
} _PACKED;


struct status_s {
	uint16	num_words;
	uint16	status;
	uint32	unused[2];
} _PACKED;


struct version_s {
	uint32	version;
	uint32	unused[3];
} _PACKED;


union packet_u {
	struct command_s	command;
	struct data_s		data;
	struct status_s		status;
	struct version_s	version;
};

#endif // VMWARE_TYPES_H
