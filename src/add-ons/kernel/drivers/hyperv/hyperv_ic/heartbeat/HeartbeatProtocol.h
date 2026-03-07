/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_HEARTBEAT_PROTOCOL_H_
#define _HYPERV_HEARTBEAT_PROTOCOL_H_


#include "ICProtocol.h"


#define HV_HEARTBEAT_RING_SIZE			0x1000
#define HV_HEARTBEAT_PKT_BUFFER_SIZE	128


// Heartbeat versions
#define HV_HEARTBEAT_VERSION_V1		MAKE_IC_VERSION(1, 0)
#define HV_HEARTBEAT_VERSION_V3		MAKE_IC_VERSION(3, 0)


static const uint32 hv_heartbeat_versions[] = {
	HV_HEARTBEAT_VERSION_V3,
	HV_HEARTBEAT_VERSION_V1
};
static const uint32 hv_heartbeat_version_count = sizeof(hv_heartbeat_versions)
	/ sizeof(hv_heartbeat_versions[0]);


// Heartbeat application states
enum {
	HV_HEARTBEAT_APPSTATE_NONE		= 0,
	HV_HEARTBEAT_APPSTATE_OK		= 1,
	HV_HEARTBEAT_APPSTATE_CRITICAL	= 2
};


// Heartbeat message
typedef struct {
	hv_ic_msg_header header;

	uint64	sequence;
	uint64	app_state;
} _PACKED hv_heartbeat_msg;


#endif // _HYPERV_HEARTBEAT_PROTOCOL_H_
