/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_SPEC_H_
#define _HYPERV_SPEC_H_


#include <SupportDefs.h>


#define HV_PAGE_SIZE					4096
#define HV_PAGE_MASK					(HV_PAGE_SIZE - 1)
#define HV_PAGE_SHIFT					12
#define HV_PAGE_ALIGN(x)				(((x) + (HV_PAGE_SIZE - 1)) & ~(HV_PAGE_SIZE - 1))
#define HV_BYTES_TO_PAGES(x)			(HV_PAGE_ALIGN(x) >> HV_PAGE_SHIFT)
#define HV_BYTES_TO_SPAN_PAGES(a, l)	((((l) - 1) >> HV_PAGE_SHIFT) \
	+ (((((l) - 1) & HV_PAGE_MASK) + ((a) & HV_PAGE_MASK)) >> HV_PAGE_SHIFT) + 1);
#define HV_MS_TO_US(x)					((x) * 1000LL)

#define HV_STATIC_ASSERT(cond, msg)	static_assert(cond, msg)


#define MAKE_VMBUS_VERSION(maj, min)	((((maj) << 16) & 0xFFFF0000) | ((min) & 0x0000FFFF))
#define GET_VMBUS_VERSION_MAJOR(ver)	(((ver) >> 16) & 0xFFFF)
#define GET_VMBUS_VERSION_MINOR(ver)	((ver) & 0xFFFF)

#define VMBUS_VERSION_WS2008				MAKE_VMBUS_VERSION(0, 13)
#define VMBUS_VERSION_WS2008R2				MAKE_VMBUS_VERSION(1, 1)
#define VMBUS_VERSION_WIN8_WS2012			MAKE_VMBUS_VERSION(2, 4)
#define VMBUS_VERSION_WIN81_WS2012R2		MAKE_VMBUS_VERSION(3, 0)
#define VMBUS_VERSION_WIN10_RS1_WS2016		MAKE_VMBUS_VERSION(4, 0)
#define VMBUS_VERSION_WIN10_RS3				MAKE_VMBUS_VERSION(4, 1)
#define VMBUS_VERSION_WIN10_V5				MAKE_VMBUS_VERSION(5, 0)
#define VMBUS_VERSION_WIN10_RS4				MAKE_VMBUS_VERSION(5, 1)
#define VMBUS_VERSION_WIN10_RS5_WS2019		MAKE_VMBUS_VERSION(5, 2)
#define VMBUS_VERSION_WS2022				MAKE_VMBUS_VERSION(5, 3)


// VMBus device types
#define VMBUS_TYPE_AVMA				"3375baf4-9e15-4b30-b765-67acb10d607b"
#define VMBUS_TYPE_BALLOON			"525074dc-8985-46e2-8057-a307dc18a502"
#define VMBUS_TYPE_DISPLAY			"da0a7802-e377-4aac-8e77-0558eb1073f8"
#define VMBUS_TYPE_FIBRECHANNEL		"2f9bcc4a-0069-4af3-b76b-6fd0be528cda"
#define VMBUS_TYPE_FILECOPY			"34d14be3-dee4-41c8-9ae7-6b174977c192"
#define VMBUS_TYPE_HEARTBEAT		"57164f39-9115-4e78-ab55-382f3bd5422d"
#define VMBUS_TYPE_IDE				"32412632-86cb-44a2-9b5c-50d1417354f5"
#define VMBUS_TYPE_INPUT			"cfa8b69e-5b4a-4cc0-b98b-8ba1a1f3f95a"
#define VMBUS_TYPE_KEYBOARD			"f912ad6d-2b17-48ea-bd65-f927a61c7684"
#define VMBUS_TYPE_KVP				"a9a0f4e7-5a45-4d96-b827-8a841e8c03e6"
#define VMBUS_TYPE_NETWORK			"f8615163-df3e-46c5-913f-f2d2f965ed0e"
#define VMBUS_TYPE_PCI				"44c4f61d-4444-4400-9d52-802e27ede19f"
#define VMBUS_TYPE_RDCONTROL		"f8e65716-3cb3-4a06-9a60-1889c5cccab5"
#define VMBUS_TYPE_RDMA				"8c2eaf3d-32a7-4b09-ab99-bd1f1c86b501"
#define VMBUS_TYPE_RDVIRT			"276aacf4-ac15-426c-98dd-7521ad3f01fe"
#define VMBUS_TYPE_SCSI				"ba6163d9-04a1-4d29-b605-72e2ffb1dc7f"
#define VMBUS_TYPE_SHUTDOWN			"0e0b6031-5213-4934-818b-38d90ced39db"
#define VMBUS_TYPE_TIMESYNC			"9527e630-d0ae-497b-adce-e80ab0175caf"
#define VMBUS_TYPE_VSS				"35fa2e29-ea23-4236-96ae-3a6ebacba440"


typedef struct {
	uint32	data1;
	uint16	data2;
	uint16	data3;
	uint8	data4[8];
} _PACKED vmbus_guid_t;


// VMBus packet types
enum {
	VMBUS_PKTTYPE_INVALID					= 0,
	VMBUS_PKTTYPE_SYNCH						= 1,
	VMBUS_PKTTYPE_ADD_TRANSFER_PAGESET		= 2,
	VMBUS_PKTTYPE_REMOVE_TRANSFER_PAGESET	= 3,
	VMBUS_PKTTYPE_CREATE_GPADL				= 4,
	VMBUS_PKTTYPE_FREE_GPADL				= 5,
	VMBUS_PKTTYPE_DATA_INBAND				= 6,
	VMBUS_PKTTYPE_DATA_USING_TRANSFER_PAGES	= 7,
	VMBUS_PKTTYPE_DATA_USING_GPADL			= 8,
	VMBUS_PKTTYPE_DATA_USING_GPA_DIRECT		= 9,
	VMBUS_PKTTYPE_CANCEL_REQUEST			= 10,
	VMBUS_PKTTYPE_COMPLETION				= 11,
	VMBUS_PKTTYPE_DATA_USING_ADDT_PKT		= 12,
	VMBUS_PKTTYPE_ADDT_DATA					= 13,
	VMBUS_PKTTYPE_MAX
};


#define VMBUS_PKT_FLAGS_RESPONSE_REQUIRED		(1 << 0)

#define VMBUS_PKT_SIZE_SHIFT	3
#define VMBUS_PKT_ALIGN(x)		(((x) + (sizeof(uint64) - 1)) &~ (sizeof(uint64) - 1))


// VMBus packet header
typedef struct {
	uint16	type;
	uint16	header_length;
	uint16	total_length;
	uint16	flags;
	uint64	transaction_id;
} _PACKED vmbus_pkt_header;


// VMBus transfer range
// Describes a buffer within an existing defined buffer
typedef struct {
	uint32	length;
	uint32	offset;
} _PACKED vmbus_transfer_range;


// VMBus transfer ranges packet header
typedef struct {
	vmbus_pkt_header	header;

	uint16	transfer_pageset_id;
	uint8	sender_owns_sets;
	uint8	reserved;

	uint32					range_count;
	vmbus_transfer_range	ranges[];
} _PACKED vmbus_pkt_transfer_header;


// VMBus GPA (Guest Physical Address) range
// Describes a buffer made up of one or more physical pages in the guest
typedef struct {
	uint32	length;
	uint32	offset;
	uint64	page_nums[];
} _PACKED vmbus_gpa_range;


// VMBus GPA packet header
typedef struct {
	vmbus_pkt_header	header;

	uint32				reserved;
	uint32				range_count;
	vmbus_gpa_range		ranges[];
} _PACKED vmbus_pkt_gpa_header;


#endif // _HYPERV_SPEC_H_
