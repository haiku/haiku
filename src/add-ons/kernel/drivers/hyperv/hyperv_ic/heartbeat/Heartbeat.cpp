/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Heartbeat.h"


Heartbeat::Heartbeat(device_node* node)
	: ICBase(node, HV_HEARTBEAT_PKT_BUFFER_SIZE, HV_IC_MSGTYPE_HEARTBEAT, hv_heartbeat_versions,
		hv_heartbeat_version_count)
{
	CALLED();
	fStatus = Connect(HV_HEARTBEAT_RING_SIZE, HV_HEARTBEAT_RING_SIZE);
}


void
Heartbeat::OnProtocolNegotiated()
{
	TRACE("Heartbeat protocol version %u.%u\n", GET_IC_VERSION_MAJOR(fMessageVersion),
		GET_IC_VERSION_MINOR(fMessageVersion));
}


void
Heartbeat::OnMessageReceived(hv_ic_msg* icMessage)
{
	hv_heartbeat_msg* message = reinterpret_cast<hv_heartbeat_msg*>(icMessage);
	if (message->header.data_length < offsetof(hv_heartbeat_msg, sequence)
			- sizeof(message->header)) {
		ERROR("Heartbeat msg invalid length 0x%X\n", message->header.data_length);
		message->header.status = HV_IC_STATUS_FAILED;
		return;
	}

	// Guest and host alternate incrementing the sequence number, typically every two seconds
	// Hyper-V will report the guest as healthy so long as this continues
	TRACE("Heartbeat sequence %" B_PRIu64 "\n", message->sequence);
	message->sequence++;
}
