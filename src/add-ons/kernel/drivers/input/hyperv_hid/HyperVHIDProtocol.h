/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_HID_PROTOCOL_H_
#define _HYPERV_HID_PROTOCOL_H_


#include <hyperv_spec.h>


#define HV_HID_RING_SIZE					0x8000
#define HV_HID_RX_PKT_BUFFER_SIZE			256
#define HV_HID_REQUEST_TRANS_ID				0xCAFECAFE
#define HV_HID_TIMEOUT_US					HV_MS_TO_US(5000)


typedef struct {
	uint8	length;
	uint8	type;
	uint16	version;
	uint8	country_code;
	uint8	num_descriptors;
	uint8	hid_descriptor_type;
	uint16	hid_descriptor_length;
} _PACKED hv_hid_descriptor;


typedef struct {
	uint32	length;
	uint16	vendor_id;
	uint16	product_id;
	uint16	version;
	uint16	reserved[11];
} _PACKED hv_hid_dev_info;


// Hyper-V HID version
#define MAKE_HID_VERSION(major, minor)	((((major) << 16) & 0xFFFF0000) | ((minor) & 0x0000FFFF))
#define GET_HID_VERSION_MAJOR(version)	(((version) >> 16) & 0xFFFF)
#define GET_HID_VERSION_MINOR(version)	((version) & 0xFFFF)

// HID protocol version 2.0 used in Server 2008 and newer.
#define HV_HID_VERSION_V2_0				MAKE_HID_VERSION(2, 0)


// HID pipe message types
enum {
	HV_HID_PIPE_MSGTYPE_INVALID		= 0,
	HV_HID_PIPE_MSGTYPE_DATA		= 1
};


// HID message types
enum {
	HV_HID_MSGTYPE_PROTOCOL_REQUEST			= 0,
	HV_HID_MSGTYPE_PROTOCOL_RESPONSE		= 1,
	HV_HID_MSGTYPE_INITIAL_DEV_INFO			= 2,
	HV_HID_MSGTYPE_INITIAL_DEV_INFO_ACK		= 3,
	HV_HID_MSGTYPE_INPUT_REPORT				= 4,
	HV_HID_MSGTYPE_INPUT_MAX
};


// HID message header
typedef struct {
	uint32	type;
	uint32	length;
} _PACKED hv_hid_msg_header;


// HID protocol request message sent to Hyper-V
typedef struct {
	hv_hid_msg_header	header;

	uint32	version;
} _PACKED hv_hid_msg_protocol_request;


// HID protocol response message received from Hyper-V
typedef struct {
	hv_hid_msg_header	header;

	uint32	version;
	uint8	result;
	uint8	reserved[3];
} _PACKED hv_hid_msg_protocol_response;


// HID initial device info message received from Hyper-V
typedef struct {
	hv_hid_msg_header	header;

	hv_hid_dev_info		info;
	hv_hid_descriptor	descriptor;
	uint8				descriptor_data[];
} _PACKED hv_hid_msg_initial_dev_info;


// HID initial device info acknowledgement message sent to Hyper-V
typedef struct {
	hv_hid_msg_header	header;

	uint8	reserved;
} _PACKED hv_hid_msg_initial_dev_info_ack;


// HID input report message received from Hyper-V
typedef struct {
	hv_hid_msg_header	header;

	uint8	data[];
} _PACKED hv_hid_msg_input_report;


// HID pipe message header
typedef struct {
	uint32	type;
	uint32	length;
} _PACKED hv_hid_pipe_msg_header;


// HID pipe message sent to Hyper-V
typedef struct {
	hv_hid_pipe_msg_header	pipe_header;

	union {
		hv_hid_msg_header					header;
		hv_hid_msg_protocol_request			protocol_req;
		hv_hid_msg_protocol_response		protocol_resp;
		hv_hid_msg_initial_dev_info_ack		dev_info_ack;
	};
} _PACKED hv_hid_pipe_out_msg;


// HID pipe message received from Hyper-V
typedef struct {
	hv_hid_pipe_msg_header	pipe_header;

	union {
		hv_hid_msg_header					header;
		hv_hid_msg_protocol_request			protocol_req;
		hv_hid_msg_protocol_response		protocol_resp;
		hv_hid_msg_initial_dev_info			dev_info;
		hv_hid_msg_initial_dev_info_ack		dev_info_ack;
		hv_hid_msg_input_report				input_report;
	};
} _PACKED hv_hid_pipe_in_msg;


typedef struct {
	uint8	buttons;
	uint16	x;
	uint16	y;
	int8	wheel;
} _PACKED hyperv_hid_input_report;


#endif // _HYPERV_HID_PROTOCOL_H_
