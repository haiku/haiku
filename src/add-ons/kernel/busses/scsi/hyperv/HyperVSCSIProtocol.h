/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_SCSI_PROTOCOL_H_
#define _HYPERV_SCSI_PROTOCOL_H_


#include <hyperv_spec.h>


#define HV_SCSI_RING_SIZE			0x200000
#define HV_SCSI_RX_PKT_BUFFER_SIZE	512

#define HV_SCSI_MAX_SCSI_DEVICES	64U
#define HV_SCSI_MAX_IDE_DEVICES		1U

#define HV_SCSI_CDB_SIZE			0x10
#define HV_SCSI_SENSE_SIZE			0x14
#define HV_SCSI_LEGACY_SENSE_SIZE	0x12

#define HV_SCSI_MAX_BLOCK_SIZE		2048
#define HV_SCSI_MAX_BLOCK_COUNT		128
#define HV_SCSI_MAX_BUFFER_SIZE		0x40000
#define HV_SCSI_MAX_BUFFER_SEGMENTS	(HV_SCSI_MAX_BUFFER_SIZE / HV_PAGE_SIZE)
#define HV_SCSI_MAX_BUFFER_PAGES	(HV_SCSI_MAX_BUFFER_SEGMENTS + 1)

#define HV_SCSI_TIMEOUT_US			HV_MS_TO_US(10000)
#define HV_SCSI_MAX_REQUESTS		2


// Hyper-V SCSI version
#define MAKE_SCSI_VERSION(major, minor)	((((major) << 8) & 0xFF00) | ((minor) & 0x00FF))
#define GET_SCSI_VERSION_MAJOR(version)	(((version) >> 8) & 0xFF)
#define GET_SCSI_VERSION_MINOR(version)	((version) & 0xFF)

#define HV_SCSI_VERSION_WIN2008			MAKE_SCSI_VERSION(2, 0)
#define HV_SCSI_VERSION_WIN2008R2		MAKE_SCSI_VERSION(4, 2)
#define HV_SCSI_VERSION_WIN8			MAKE_SCSI_VERSION(5, 1)
#define HV_SCSI_VERSION_WIN81			MAKE_SCSI_VERSION(6, 0)
#define HV_SCSI_VERSION_WIN10			MAKE_SCSI_VERSION(6, 1)


static const uint32 hv_scsi_versions[] = {
	HV_SCSI_VERSION_WIN10,
	HV_SCSI_VERSION_WIN81,
	HV_SCSI_VERSION_WIN8,
	HV_SCSI_VERSION_WIN2008R2,
	HV_SCSI_VERSION_WIN2008
};
static const uint32 hv_scsi_version_count = sizeof(hv_scsi_versions)
	/ sizeof(hv_scsi_versions[0]);


// SCSI request direction
enum {
	HV_SCSI_DIRECTION_WRITE		= 0,
	HV_SCSI_DIRECTION_READ		= 1,
	HV_SCSI_DIRECTION_UNKNOWN	= 2
};


#define HV_SCSI_SRB_FLAGS_DISABLE_SYNC_TRANSFER	(1 << 3)
#define HV_SCSI_SRB_FLAGS_DISABLE_AUTOSENSE		(1 << 5)
#define HV_SCSI_SRB_FLAGS_DATA_IN				(1 << 6)
#define HV_SCSI_SRB_FLAGS_DATA_OUT				(1 << 7)


// SCSI request extension on Windows 8 / Windows Server 2012 and newer
typedef struct {
	uint16	reserved;
	uint8	queue_tag;
	uint8	queue_action;
	uint32	srb_flags;
	uint32	timeout;
	uint32	queue_sort_by;
} _PACKED hv_scsi_request_win8_extension;


// SCSI message types
enum {
	HV_SCSI_MSGTYPE_COMPLETE_IO				= 1,
	HV_SCSI_MSGTYPE_REMOVE_DEVICE			= 2,
	HV_SCSI_MSGTYPE_EXECUTE_SRB				= 3,
	HV_SCSI_MSGTYPE_RESET_LUN				= 4,
	HV_SCSI_MSGTYPE_RESET_ADAPTER			= 5,
	HV_SCSI_MSGTYPE_RESET_BUS				= 6,
	HV_SCSI_MSGTYPE_BEGIN_INIT				= 7,
	HV_SCSI_MSGTYPE_END_INIT				= 8,
	HV_SCSI_MSGTYPE_QUERY_PROTOCOL_VER		= 9,
	HV_SCSI_MSGTYPE_QUERY_PROPERTIES		= 10,
	HV_SCSI_MSGTYPE_ENUM_BUS				= 11,
	HV_SCSI_MSGTYPE_FCHBA_DATA				= 12,
	HV_SCSI_MSGTYPE_CREATE_SUB_CHANNELS		= 13
};


#define HV_SCSI_FLAG_REQUEST_COMPLETION		1
#define HV_SCSI_SUCCESS						0


// SCSI message header
typedef struct {
	uint32	type;
	uint32	flags;
	uint32	status;
} _PACKED hv_scsi_msg_header;


// SCSI request message sent to Hyper-V
typedef struct {
	hv_scsi_msg_header	header;

	uint16	length;
	uint8	srb_status;
	uint8	scsi_status;

	uint8	port;
	uint8	path_id;
	uint8	target_id;
	uint8	lun;

	uint8	cdb_length;
	uint8	sense_info_length;
	uint8	direction;
	uint8	reserved;
	uint32	data_length;

	union {
		uint8	cdb[HV_SCSI_CDB_SIZE];
		uint8	sense[HV_SCSI_SENSE_SIZE];
	};

	hv_scsi_request_win8_extension win8_extension;
} _PACKED hv_scsi_msg_request;


#define HV_SCSI_FLAG_SUPPORTS_MULTI_CHANNEL		1

// SCSI channel properties message
typedef struct {
	hv_scsi_msg_header	header;

	uint32	reserved1;
	uint16	max_channels;
	uint16	reserved2;

	uint32	flags;
	uint32	max_transfer_bytes;

	uint64	reserved3;
} _PACKED hv_scsi_msg_channel_properties;


// SCSI protocol message
typedef struct {
	hv_scsi_msg_header	header;

	uint16	version;
	uint16	revision; // Always zero for this driver
} _PACKED hv_scsi_msg_protocol;


// SCSI Fibre Channel WWN message
typedef struct {
	uint8	primary_active;
	uint8	reserved[3];
	uint8	primary_port_wwn[8];
	uint8	primary_node_wwn[8];
	uint8	secondary_port_wwn[8];
	uint8	secondary_node_wwn[8];
} _PACKED hv_scsi_msg_fibre_channel_wwn;


// SCSI combined message
typedef union {
	hv_scsi_msg_header				header;
	hv_scsi_msg_request				request;
	hv_scsi_msg_channel_properties	channel_properties;
	hv_scsi_msg_protocol			protocol;
	hv_scsi_msg_fibre_channel_wwn	fibre_channel_wwn;

	uint16							sub_channels;
	uint8							padding[sizeof(hv_scsi_msg_header) + 0x34];
} _PACKED hv_scsi_msg;


#endif
