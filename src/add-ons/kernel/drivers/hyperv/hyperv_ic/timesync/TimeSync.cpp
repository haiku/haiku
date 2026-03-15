/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <real_time_clock.h>

#include "TimeSync.h"


TimeSync::TimeSync(device_node* node)
	: ICBase(node, HV_TIMESYNC_PKT_BUFFER_SIZE, HV_IC_MSGTYPE_TIMESYNC, hv_timesync_versions,
		hv_timesync_version_count)
{
	CALLED();
	fStatus = Connect(HV_TIMESYNC_RING_SIZE, HV_TIMESYNC_RING_SIZE);
}


void
TimeSync::OnProtocolNegotiated()
{
	TRACE("Time sync protocol version %u.%u\n", GET_IC_VERSION_MAJOR(fMessageVersion),
		GET_IC_VERSION_MINOR(fMessageVersion));
}


void
TimeSync::OnMessageReceived(hv_ic_msg* icMessage)
{
	if (fMessageVersion >= HV_TIMESYNC_VERSION_V4_0) {
		hv_timesync_msg_v4* message = reinterpret_cast<hv_timesync_msg_v4*>(icMessage);
		if (message->header.data_length < offsetof(hv_timesync_msg_v4, stratum)
				- sizeof(message->header)) {
			ERROR("Time sync msg invalid length 0x%X\n", message->header.data_length);
			message->header.status = HV_IC_STATUS_FAILED;
			return;
		}
		_SyncTime(message->parent_time, message->reference_time, message->flags);
	} else {
		hv_timesync_msg_v1* message = reinterpret_cast<hv_timesync_msg_v1*>(icMessage);
		if (message->header.data_length < offsetof(hv_timesync_msg_v1, flags)
				- sizeof(message->header)) {
			ERROR("Time sync msg invalid length 0x%X\n", message->header.data_length);
			message->header.status = HV_IC_STATUS_FAILED;
			return;
		}
		_SyncTime(message->parent_time, 0, message->flags);
	}
}


void
TimeSync::_SyncTime(uint64 hostTime, uint64 referenceTime, uint8 flags)
{
	TRACE("New host time %" B_PRIu64 " ref time %" B_PRIu64 " flags 0x%X\n", hostTime,
		referenceTime, flags);
	if ((flags & (HV_TIMESYNC_MSGFLAGS_SYNC | HV_TIMESYNC_MSGFLAGS_SAMPLE)) == 0)
		return;

	// Calculate adjustment based on reference counter if supported
	uint64 timeAdjust = 0;
	if (referenceTime != 0) {
		uint64 refCount;
		if (fHyperV->get_reference_counter(fHyperVCookie, &refCount) == B_OK) {
			timeAdjust = refCount - referenceTime;
			TRACE("Reference counter adjustment by %" B_PRIu64 "\n", timeAdjust);
		}
	}

	// Time provided by Hyper-V is always UTC
	bigtime_t newTime = (hostTime - HV_TIMESYNC_TIME_BASE + timeAdjust) / 10LL;
	TRACE("New time %" B_PRIdBIGTIME "\n", newTime);

	if ((flags & HV_TIMESYNC_MSGFLAGS_SAMPLE) != 0) {
		bigtime_t currentTime = real_time_clock_usecs();
		TRACE("Current time %" B_PRIdBIGTIME "\n", currentTime);

		bigtime_t diffTime = (newTime > currentTime) ? (newTime - currentTime)
			: (currentTime - newTime);
		TRACE("Time diff %" B_PRIdBIGTIME "\n", diffTime);
		if (diffTime <= HV_MS_TO_US(500))
			return;
	}

	// System time will be changed if explicitly requested, or the difference is too great
	TRACE("Set system time to %" B_PRIdBIGTIME "\n", newTime);
	set_real_time_clock_usecs(newTime);
}
