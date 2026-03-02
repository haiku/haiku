/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_SPEC_PRIVATE_H_
#define _HYPERV_SPEC_PRIVATE_H_


#include <hyperv_spec.h>


// Hyper-V message types
enum {
	HYPERV_MSGTYPE_NONE				= 0x00000000,
	HYPERV_MSGTYPE_CHANNEL			= 0x00000001,
	HYPERV_MSGTYPE_TIMER_EXPIRED	= 0x80000010
};


// Hypercall status
enum {
	HYPERCALL_STATUS_SUCCESS						= 0x0000,
	HYPERCALL_STATUS_INVALID_HYPERCALL_CODE			= 0x0002,
	HYPERCALL_STATUS_INVALID_HYPERCALL_INPUT		= 0x0003,
	HYPERCALL_STATUS_INVALID_ALIGNMENT				= 0x0004,
	HYPERCALL_STATUS_INVALID_PARAMETER				= 0x0005,
	HYPERCALL_STATUS_ACCESS_DENIED					= 0x0006,
	HYPERCALL_STATUS_INVALID_PARTITION_STATE		= 0x0007,
	HYPERCALL_STATUS_OPERATION_DENIED				= 0x0008,
	HYPERCALL_STATUS_UNKNOWN_PROPERTY				= 0x0009,
	HYPERCALL_STATUS_PROPERTY_VALUE_OUT_OF_RANGE	= 0x000A,
	HYPERCALL_STATUS_INSUFFICIENT_MEMORY			= 0x000B,
	HYPERCALL_STATUS_PARTITION_TOO_DEEP				= 0x000C,
	HYPERCALL_STATUS_INVALID_PARTITION_ID			= 0x000D,
	HYPERCALL_STATUS_INVALID_VP_INDEX				= 0x000E,
	HYPERCALL_STATUS_INVALID_PORT_ID				= 0x0011,
	HYPERCALL_STATUS_INVALID_CONNECTION_ID			= 0x0012,
	HYPERCALL_STATUS_INSUFFICIENT_BUFFERS			= 0x0013,
	HYPERCALL_STATUS_NOT_ACKNOWLEDGED				= 0x0014,
	HYPERCALL_STATUS_ACKNOWLEDGED					= 0x0016,
	HYPERCALL_STATUS_INVALID_SAVE_RESTORE_STATE		= 0x0017,
	HYPERCALL_STATUS_INVALID_SYNIC_STATE			= 0x0018,
	HYPERCALL_STATUS_OBJECT_IN_USE					= 0x0019,
	HYPERCALL_STATUS_INVALID_PROXIMITY_DOMAIN_INFO	= 0x001A,
	HYPERCALL_STATUS_NO_DATA						= 0x001B,
	HYPERCALL_STATUS_INACTIVE						= 0x001C,
	HYPERCALL_STATUS_NO_RESOURCES					= 0x001D,
	HYPERCALL_STATUS_FEATURE_UNAVAILABLE			= 0x001E,
	HYPERCALL_STATUS_PARTIAL_PACKET					= 0x001F
};


// Slow memory-based hypercall for VMBus messaging
#define HYPERCALL_POST_MESSAGE			0x0005C
// Fast register-based hypercall for VMBus events
#define HYPERCALL_SIGNAL_EVENT			0x1005D

// Maximum size for a hypercall message
#define HYPERCALL_MAX_DATA_SIZE			240
#define HYPERCALL_MAX_SIZE				256
// Maximum hypercall retry count
#define HYPERCALL_MAX_RETRY_COUNT		20


// Hypercall post message input parameters
// https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/hypercalls/hvcallpostmessage
typedef struct {
	uint32	connection_id;
	uint32	reserved;
	uint32	message_type;
	uint32	data_size;
	uint8	data[HYPERCALL_MAX_DATA_SIZE];
} _PACKED hypercall_post_msg_input;


#define HYPERV_SYNIC_MAX_INTS		16

// SynIC message page
// https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/inter-partition-communication#sim-page

#define HV_MESSAGE_FLAGS_PENDING	(1 << 0)
#define HV_MESSAGE_DATA_SIZE		240
#define HV_MESSAGE_SIZE 			256


// Per-interrupt message data
typedef struct {
	uint32	message_type;
	uint8	payload_size;
	uint8	message_flags;
	uint16	reserved1;
	union {
		uint64	origination_id;
		struct {
			uint32	port_id 	: 24;
			uint32	reserved2 	: 8;
		};
	};

	uint8	data[HV_MESSAGE_DATA_SIZE];
} _PACKED hv_message;


// All interrupts message data
typedef struct {
	hv_message	interrupts[HYPERV_SYNIC_MAX_INTS];
} hv_message_page;
HV_STATIC_ASSERT(sizeof(hv_message_page) == HV_PAGE_SIZE, "hv_message_page size mismatch");


// SynIC event flags
// https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/inter-partition-communication#synic-event-flags

#define HV_EVENT_FLAGS_SIZE		256
#define HV_EVENT_FLAGS_COUNT	(HV_EVENT_FLAGS_SIZE * 8)


// Per-interrupt event flags
typedef union {
	uint8	flags[HV_EVENT_FLAGS_SIZE];
	uint32	flags32[HV_EVENT_FLAGS_SIZE / sizeof(uint32)];
} hv_event_flags;


// All interrupts event flags
typedef struct {
	hv_event_flags	interrupts[HYPERV_SYNIC_MAX_INTS];
} hv_event_flags_page;
HV_STATIC_ASSERT(sizeof(hv_event_flags_page) == HV_PAGE_SIZE, "hv_event_flags_page size mismatch");


// HID of VMBus ACPI device
// This is normally just "VMBus", but acpica seems to need all caps
#define VMBUS_ACPI_HID_NAME		"VMBUS"

// Fixed interrupt for VMBus messages
#define VMBUS_SINT_MESSAGE		2
// Fixed interrupt for VMBus timers
#define VMBUS_SINT_TIMER		4

// Fixed connection ID for messages
#define VMBUS_CONNID_MESSAGE		1
// Fixed connection ID for events
#define VMBUS_CONNID_EVENTS			2

// Max channels is 2048 on Server 2012 and newer
// Server 2008 and 2008 R2 have a maximum of 256
#define VMBUS_MAX_CHANNELS			HV_EVENT_FLAGS_COUNT
#define VMBUS_MAX_CHANNELS_LEGACY	256

// Channel IDs start at 1, 0 is considered an invalid channel ID by Hyper-V
#define VMBUS_CHANNEL_ID		0


// Ordered list of newest to oldest VMBus versions when connecting
static const uint32 vmbus_versions[] = {
	VMBUS_VERSION_WS2022,
	VMBUS_VERSION_WIN10_RS5_WS2019,
	VMBUS_VERSION_WIN10_RS4,
	VMBUS_VERSION_WIN10_V5,
	VMBUS_VERSION_WIN10_RS3,
	VMBUS_VERSION_WIN10_RS1_WS2016,
	VMBUS_VERSION_WIN81_WS2012R2,
	VMBUS_VERSION_WIN8_WS2012,
	VMBUS_VERSION_WS2008R2,
	VMBUS_VERSION_WS2008
};


 // VMBus GPADL range descriptor
typedef struct {
	uint32	length;
	uint32	offset;
	uint64	page_nums[];
} _PACKED vmbus_gpadl_range;


#define VMBUS_GPADL_NULL		0
#define VMBUS_GPADL_MAX_PAGES	8192


// VMBus RX and TX event flags page
typedef struct {
	hv_event_flags	rx_event_flags;
	uint8			reserved1[(HV_PAGE_SIZE / 2) - sizeof(rx_event_flags)];
	hv_event_flags	tx_event_flags;
	uint8			reserved2[(HV_PAGE_SIZE / 2) - sizeof(tx_event_flags)];
} vmbus_event_flags_page;
HV_STATIC_ASSERT(sizeof(vmbus_event_flags_page) == HV_PAGE_SIZE,
	"vmbus_event_flags_page size mismatch");

 // VMBus message type
enum {
	VMBUS_MSGTYPE_INVALID					= 0,
	VMBUS_MSGTYPE_CHANNEL_OFFER				= 1,
	VMBUS_MSGTYPE_RESCIND_CHANNEL_OFFER		= 2,
	VMBUS_MSGTYPE_REQUEST_CHANNELS			= 3,
	VMBUS_MSGTYPE_REQUEST_CHANNELS_DONE		= 4,
	VMBUS_MSGTYPE_OPEN_CHANNEL				= 5,
	VMBUS_MSGTYPE_OPEN_CHANNEL_RESPONSE		= 6,
	VMBUS_MSGTYPE_CLOSE_CHANNEL				= 7,
	VMBUS_MSGTYPE_CREATE_GPADL				= 8,
	VMBUS_MSGTYPE_CREATE_GPADL_ADDITIONAL	= 9,
	VMBUS_MSGTYPE_CREATE_GPADL_RESPONSE		= 10,
	VMBUS_MSGTYPE_FREE_GPADL				= 11,
	VMBUS_MSGTYPE_FREE_GPADL_RESPONSE		= 12,
	VMBUS_MSGTYPE_FREE_CHANNEL				= 13,
	VMBUS_MSGTYPE_CONNECT					= 14,
	VMBUS_MSGTYPE_CONNECT_RESPONSE			= 15,
	VMBUS_MSGTYPE_DISCONNECT				= 16,
	VMBUS_MSGTYPE_DISCONNECT_RESPONSE		= 17,
	VMBUS_MSGTYPE_MODIFY_CHANNEL			= 22,
	VMBUS_MSGTYPE_MODIFY_CHANNEL_RESPONSE	= 24,
	VMBUS_MSGTYPE_MAX
};


// VMBus message header
typedef struct {
	uint32	type;
	uint32	reserved;
} _PACKED vmbus_msg_header;


// VMBus channel offer message from Hyper-V
typedef struct {
	vmbus_msg_header header;

	vmbus_guid_t	type_id;
	vmbus_guid_t	instance_id;
	uint64			reserved1[2];
	uint16			flags;
	uint16			mmio_size_mb;

	union {
		struct {
#define VMBUS_CHANNEL_OFFER_MAX_USER_BYTES      120
			uint8	data[VMBUS_CHANNEL_OFFER_MAX_USER_BYTES];
		} standard;
		struct {
			uint32	mode;
			uint8	data[VMBUS_CHANNEL_OFFER_MAX_USER_BYTES - 4];
		} pipe;
	};

	uint16	sub_index;
	uint16	reserved2;
	uint32	channel_id;
	uint8	monitor_id;

	// Fields present only in Server 2008 R2 and newer
	uint8	monitor_alloc	: 1;
	uint8	reserved3		: 7;
	uint16	dedicated_int	: 1;
	uint16	reserved4		: 15;

	uint32	connection_id;
} _PACKED vmbus_msg_channel_offer;


// VMBus rescind channel offer message from Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint32	channel_id;
} _PACKED vmbus_msg_rescind_channel_offer;


// VMBus request channels message to Hyper-V
typedef struct {
	vmbus_msg_header header;
} _PACKED vmbus_msg_request_channels;


// VMBus request channels done message from Hyper-V
typedef struct {
	vmbus_msg_header header;
} _PACKED vmbus_msg_request_channels_done;


// VMBus open channel message to Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint32	channel_id;
	uint32	open_id;
	uint32	gpadl_id;
	uint32	target_cpu;
	uint32	rx_page_offset;
	uint8	user_data[VMBUS_CHANNEL_OFFER_MAX_USER_BYTES];
} _PACKED vmbus_msg_open_channel;


// VMBus open channel response message from Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint32	channel_id;
	uint32	open_id;
	uint32	result;
} _PACKED vmbus_msg_open_channel_resp;


// VMBus close channel message to Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint32	channel_id;
} _PACKED vmbus_msg_close_channel;


// VMBus create GPADL message to Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint32			channel_id;
	uint32			gpadl_id;
	uint16			total_range_length;
	uint16			range_count;
	vmbus_gpa_range	ranges[1]; // Only 1 range is supported by this driver
} _PACKED vmbus_msg_create_gpadl;
#define VMBUS_MSG_CREATE_GPADL_MAX_PAGES \
	((HYPERCALL_MAX_DATA_SIZE - sizeof(vmbus_msg_create_gpadl)) / sizeof(uint64))


// VMBus create GPADL additional pages message to Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint32	msg_num;
	uint32	gpadl_id;
	uint64	page_nums[];
} _PACKED vmbus_msg_create_gpadl_additional;
#define VMBUS_MSG_CREATE_GPADL_ADDITIONAL_MAX_PAGES \
	((HYPERCALL_MAX_DATA_SIZE - sizeof(vmbus_msg_create_gpadl_additional)) / sizeof(uint64))


// VMBus create GPADL response message from Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint32	channel_id;
	uint32	gpadl_id;
	uint32	result;
} _PACKED vmbus_msg_create_gpadl_resp;


// VMBus free GPADL message to Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint32	channel_id;
	uint32	gpadl_id;
} _PACKED vmbus_msg_free_gpadl;


// VMBus free GPADL message from Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint32	gpadl_id;
} _PACKED vmbus_msg_free_gpadl_resp;


// VMBus free channel message to Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint32	channel_id;
} _PACKED vmbus_msg_free_channel;


// VMBus connect message to Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint32	version;
	uint32	target_cpu;

	uint64	event_flags_physaddr;
	uint64	monitor1_physaddr;
	uint64 	monitor2_physaddr;
} _PACKED vmbus_msg_connect;


// VMBus connect response message from Hyper-V
typedef struct {
	vmbus_msg_header header;

	uint8	supported;
	uint8	connection_state;
	uint16	reserved;
	uint32	connection_id;
} _PACKED vmbus_msg_connect_resp;


// VMBus disconnect message to Hyper-V
typedef struct {
	vmbus_msg_header header;
} _PACKED vmbus_msg_disconnect;


// VMBus disconnect response message from Hyper-V
typedef struct {
	vmbus_msg_header header;
} _PACKED vmbus_msg_disconnect_resp;


// VMBus combined message
typedef union {
	vmbus_msg_header	header;

	vmbus_msg_channel_offer				channel_offer;
	vmbus_msg_rescind_channel_offer		rescind_channel_offer;
	vmbus_msg_request_channels			request_channels;
	vmbus_msg_request_channels_done		request_channels_done;
	vmbus_msg_open_channel				open_channel;
	vmbus_msg_open_channel_resp			open_channel_resp;
	vmbus_msg_close_channel				close_channel;
	vmbus_msg_create_gpadl				create_gpadl;
	vmbus_msg_create_gpadl_additional	create_gpadl_additional;
	vmbus_msg_create_gpadl_resp			create_gpadl_resp;
	vmbus_msg_free_gpadl				free_gpadl;
	vmbus_msg_free_gpadl_resp			free_gpadl_resp;
	vmbus_msg_free_channel				free_channel;
	vmbus_msg_connect					connect;
	vmbus_msg_connect_resp				connect_resp;
	vmbus_msg_disconnect				disconnect;
	vmbus_msg_disconnect_resp			disconnect_resp;
} _PACKED vmbus_msg;


// VMBus message type to message length lookup
static const uint32 vmbus_msg_lengths[VMBUS_MSGTYPE_MAX] = {
	0,											// VMBUS_MSGTYPE_INVALID
	sizeof(vmbus_msg_channel_offer),			// VMBUS_MSGTYPE_CHANNEL_OFFER
	sizeof(vmbus_msg_rescind_channel_offer),	// VMBUS_MSGTYPE_RESCIND_CHANNEL_OFFER
	sizeof(vmbus_msg_request_channels),			// VMBUS_MSGTYPE_REQUEST_CHANNELS
	sizeof(vmbus_msg_request_channels_done),	// VMBUS_MSGTYPE_REQUEST_CHANNELS_DONE
	sizeof(vmbus_msg_open_channel),				// VMBUS_MSGTYPE_OPEN_CHANNEL
	sizeof(vmbus_msg_open_channel_resp),		// VMBUS_MSGTYPE_OPEN_CHANNEL_RESPONSE
	sizeof(vmbus_msg_close_channel),			// VMBUS_MSGTYPE_CLOSE_CHANNEL
	0,											// VMBUS_MSGTYPE_CREATE_GPADL
	0,											// VMBUS_MSGTYPE_CREATE_GPADL_PAGES
	sizeof(vmbus_msg_create_gpadl_resp),		// VMBUS_MSGTYPE_CREATE_GPADL_RESPONSE
	sizeof(vmbus_msg_free_gpadl),				// VMBUS_MSGTYPE_FREE_GPADL
	sizeof(vmbus_msg_free_gpadl_resp),			// VMBUS_MSGTYPE_FREE_GPADL_RESPONSE
	sizeof(vmbus_msg_free_channel),				// VMBUS_MSGTYPE_FREE_CHANNEL
	sizeof(vmbus_msg_connect),					// VMBUS_MSGTYPE_CONNECT
	sizeof(vmbus_msg_connect_resp),				// VMBUS_MSGTYPE_CONNECT_RESPONSE
	sizeof(vmbus_msg_disconnect),				// VMBUS_MSGTYPE_DISCONNECT
	sizeof(vmbus_msg_disconnect_resp),			// VMBUS_MSGTYPE_DISCONNECT_RESPONSE
	0,											// 18
	0,											// 19
	0,											// 20
	0,											// 21
	0,											// VMBUS_MSGTYPE_MODIFY_CHANNEL
	0,											// 23
	0											// VMBUS_MSGTYPE_MODIFY_CHANNEL_RESPONSE
};


// VMBus ring buffer structure
typedef struct {
	volatile uint32	write_index;
	volatile uint32	read_index;

	volatile uint32	interrupt_mask;
	volatile uint32	pending_send_size;

	uint32	reserved[12];
	union {
		struct {
			uint32 pending_send_size_supported : 1;
		};
		uint32 value;
	} features;

	// Padding to ensure data buffer is page-aligned.
	uint8 padding[HV_PAGE_SIZE - 76];

	volatile uint64 guest_to_host_interrupt_count;

	uint8	buffer[];
} vmbus_ring_buffer;
HV_STATIC_ASSERT(sizeof(vmbus_ring_buffer) == HV_PAGE_SIZE, "vmbus_ring_buffer size mismatch");


#endif // _HYPERV_SPEC_PRIVATE_H_
