/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_IC_PROTOCOL_H_
#define _HYPERV_IC_PROTOCOL_H_


#define MAKE_IC_VERSION(major, minor)	((((minor) << 16) & 0xFFFF0000) | ((major) & 0x0000FFFF))
#define GET_IC_VERSION_MAJOR(version)	((version) & 0xFFFF)
#define GET_IC_VERSION_MINOR(version)	(((version) >> 16) & 0xFFFF)

// IC framework versions
#define HV_IC_VERSION_2008		MAKE_IC_VERSION(1, 0)
#define HV_IC_VERSION_V3		MAKE_IC_VERSION(3, 0)


static const uint32 hv_framework_versions[] = {
	HV_IC_VERSION_V3,
	HV_IC_VERSION_2008
};
static const uint32 hv_framework_version_count = sizeof(hv_framework_versions)
	/ sizeof(hv_framework_versions[0]);


// IC message types
enum {
	HV_IC_MSGTYPE_NEGOTIATE		= 0,
	HV_IC_MSGTYPE_HEARTBEAT		= 1,
	HV_IC_MSGTYPE_KVP			= 2,
	HV_IC_MSGTYPE_SHUTDOWN		= 3,
	HV_IC_MSGTYPE_TIMESYNC		= 4,
	HV_IC_MSGTYPE_VSS			= 5,
	HV_IC_MSGTYPE_FILECOPY		= 7
};


// IC message flags
#define HV_IC_FLAG_TRANSACTION		(1 << 0)
#define HV_IC_FLAG_REQUEST			(1 << 1)
#define HV_IC_FLAG_RESPONSE			(1 << 2)


// IC status
enum {
	HV_IC_STATUS_OK					= 0x0,
	HV_IC_STATUS_FAILED				= 0x80004005,
	HV_IC_STATUS_TIMEOUT			= 0x800705B4,
	HV_IC_STATUS_INVALID_ARG		= 0x80070057,
	HV_IC_STATUS_ALREADY_EXISTS		= 0x80070050,
	HV_IC_STATUS_DISK_FULL			= 0x80070070
};


// IC message header
typedef struct {
	uint32	pipe_flags;
	uint32	pipe_messages;
	uint32	framework_version;
	uint16	type;
	uint32	message_version;
	uint16	data_length;
	uint32	status;
	uint8	transaction_id;
	uint8	flags;
	uint16	reserved;
} _PACKED hv_ic_msg_header;


// IC negotiation message
typedef struct {
	hv_ic_msg_header header;

	uint16	framework_version_count;
	uint16	message_version_count;
	uint32	reserved;
	uint32	versions[];
} _PACKED hv_ic_msg_negotiate;


// IC combined message
typedef struct {
	union {
		hv_ic_msg_header	header;
		hv_ic_msg_negotiate	negotiate;
	};
} _PACKED hv_ic_msg;


#endif // _HYPERV_IC_PROTOCOL_H_
