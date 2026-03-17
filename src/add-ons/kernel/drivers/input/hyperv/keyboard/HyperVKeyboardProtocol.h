/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_KEYBOARD_PROTOCOL_H_
#define _HYPERV_KEYBOARD_PROTOCOL_H_


#include <hyperv_spec.h>


#define HV_KEYBOARD_TX_RING_SIZE		0x1000
#define HV_KEYBOARD_RX_RING_SIZE		0x8000
#define HV_KEYBOARD_RX_PKT_BUFFER_SIZE	128
#define HV_KEYBOARD_TIMEOUT_US			HV_MS_TO_US(10000)


// Hyper-V keyboard version, only one currently
#define MAKE_KEYBOARD_VERSION(major, minor)	((((major) << 16) & 0xFFFF0000) | ((minor) & 0xFFFF))
#define GET_KEYBOARD_VERSION_MAJOR(version)	(((version) >> 16) & 0xFFFF)
#define GET_KEYBOARD_VERSION_MINOR(version)	((version) & 0xFFFF)

#define HV_KEYBOARD_VERSION_V1_0		MAKE_KEYBOARD_VERSION(1, 0)


// Keyboard message types
enum {
	HV_KEYBOARD_MSGTYPE_PROTOCOL_REQUEST	= 1,
	HV_KEYBOARD_MSGTYPE_PROTOCOL_RESPONSE	= 2,
	HV_KEYBOARD_MSGTYPE_EVENT				= 3,
	HV_KEYBOARD_MSGTYPE_SET_INDICATORS		= 4
};


// Keyboard message header
typedef struct {
	uint32	type;
} _PACKED hv_keyboard_msg_header;


// Keyboard protocol request message to Hyper-V
typedef struct {
	hv_keyboard_msg_header header;

	uint32	version;
} _PACKED hv_keyboard_msg_protocol_request;


#define HV_KEYBOARD_PROTOCOL_STATUS_OK 1


// Keyboard protocol response message from Hyper-V
typedef struct {
	hv_keyboard_msg_header header;

	uint32	status;
} _PACKED hv_keyboard_msg_protocol_response;


// Keyboard event message from Hyper-V
typedef struct {
	hv_keyboard_msg_header header;

	uint16	keycode;
	uint16	reserved1;
	uint32	is_unicode : 1;
	uint32	is_break : 1;
	uint32	is_e0 : 1;
	uint32	is_e1 : 1;
	uint32	reserved2 : 28;
} _PACKED hv_keyboard_msg_event;


#define HV_KEYBOARD_INDICATORS_SCROLL_LOCK	1
#define HV_KEYBOARD_INDICATORS_NUM_LOCK		2
#define HV_KEYBOARD_INDICATORS_CAPS_LOCK	4


// Keyboard set indicators message to Hyper-V
typedef struct {
	hv_keyboard_msg_header header;

	uint16	indicators;
} _PACKED hv_keyboard_msg_set_indicators;


// Keyboard combined message
typedef union {
	hv_keyboard_msg_header				header;
	hv_keyboard_msg_protocol_request	protocol_request;
	hv_keyboard_msg_protocol_response	protocol_response;
	hv_keyboard_msg_event				event;
	hv_keyboard_msg_set_indicators		set_indicators;
} _PACKED hv_keyboard_msg;


#endif // _HYPERV_KEYBOARD_PROTOCOL_H_
