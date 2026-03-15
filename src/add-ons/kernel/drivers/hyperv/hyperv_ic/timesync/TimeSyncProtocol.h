/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_TIMESYNC_PROTOCOL_H_
#define _HYPERV_TIMESYNC_PROTOCOL_H_


#include "ICProtocol.h"


#define HV_TIMESYNC_RING_SIZE		0x1000
#define HV_TIMESYNC_PKT_BUFFER_SIZE	128


// Time sync versions
#define HV_TIMESYNC_VERSION_V1_0	MAKE_IC_VERSION(1, 0)
#define HV_TIMESYNC_VERSION_V3_0	MAKE_IC_VERSION(3, 0)
#define HV_TIMESYNC_VERSION_V4_0	MAKE_IC_VERSION(4, 0)


static const uint32 hv_timesync_versions[] = {
	HV_TIMESYNC_VERSION_V4_0,
	HV_TIMESYNC_VERSION_V3_0,
	HV_TIMESYNC_VERSION_V1_0
};
static const uint32 hv_timesync_version_count = sizeof(hv_timesync_versions)
	/ sizeof(hv_timesync_versions[0]);


#define HV_TIMESYNC_TIME_BASE		116444736000000000ULL

// Time sync message flags
#define HV_TIMESYNC_MSGFLAGS_SYNC		(1 << 0)
#define HV_TIMESYNC_MSGFLAGS_SAMPLE		(1 << 1)


// Time sync message (versions 1 and 3)
typedef struct {
	hv_ic_msg_header header;

	uint64	parent_time;
	uint64	child_time;
	uint64	round_trip_time;
	uint8	flags;
} _PACKED hv_timesync_msg_v1;


// Time sync message (version 4)
typedef struct {
	hv_ic_msg_header header;

	uint64	parent_time;
	uint64	reference_time;
	uint8	flags;
	uint8	leap_flags;
	uint8	stratum;
	uint8	reserved[3];
} _PACKED hv_timesync_msg_v4;


#endif