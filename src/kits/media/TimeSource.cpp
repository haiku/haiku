/*
 * Copyright 2002-2012, Haiku. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *    Dario Casalinuovo
 *    Marcus Overhagen
 */


#include <TimeSource.h>

#include <Autolock.h>

#include <string.h>

#include "MediaDebug.h"
#include "DataExchange.h"
#include "ServerInterface.h"
#include "TimeSourceObject.h"
#include "TMap.h"

#define DEBUG_TIMESOURCE 0

#if DEBUG_TIMESOURCE
	#define TRACE_TIMESOURCE printf
#else
	#define TRACE_TIMESOURCE if (1) {} else printf
#endif

namespace BPrivate { namespace media {

#define _atomic_read(p) 	atomic_or((p), 0)

// must be multiple of page size
#define TS_AREA_SIZE		B_PAGE_SIZE
// must be power of two
#define TS_INDEX_COUNT		128

// sizeof(TimeSourceTransmit) must be <= TS_AREA_SIZE
struct TimeSourceTransmit
{
	int32 readindex;
	int32 writeindex;
	int32 isrunning;
	bigtime_t realtime[TS_INDEX_COUNT];
	bigtime_t perftime[TS_INDEX_COUNT];
	float drift[TS_INDEX_COUNT];
};

#define MAX_SLAVE_NODES 300


class SlaveNodes : public BLocker
{
public:
								SlaveNodes();
								~SlaveNodes();

	int32						CountSlaves() const;
	bool						GetNextSlave(port_id** id);
	void						Rewind();

	bool						InsertSlave(const media_node& node);
	bool						RemoveSlave(const media_node& node);
private:
	Map<media_node_id, port_id>	fSlaveList;
};


SlaveNodes::SlaveNodes()
	:
	BLocker("BTimeSource slavenodes")
{
}


SlaveNodes::~SlaveNodes()
{
	fSlaveList.MakeEmpty();
}


int32
SlaveNodes::CountSlaves() const
{
	return fSlaveList.CountItems();
}


bool
SlaveNodes::GetNextSlave(port_id** id)
{
	return fSlaveList.GetNext(id);
}


void
SlaveNodes::Rewind()
{
	fSlaveList.Rewind();
}


bool
SlaveNodes::InsertSlave(const media_node& node)
{
	return fSlaveList.Insert(node.node, node.port);
}


bool
SlaveNodes::RemoveSlave(const media_node& node)
{
	return fSlaveList.Remove(node.node);
}


} } // namespace BPrivate::media


/*************************************************************
 * protected BTimeSource
 *************************************************************/

BTimeSource::~BTimeSource()
{
	CALLED();
	if (fArea > 0)
		delete_area(fArea);
	delete fSlaveNodes;
}

/*************************************************************
 * public BTimeSource
 *************************************************************/

status_t
BTimeSource::SnoozeUntil(bigtime_t performance_time,
	bigtime_t with_latency, bool retry_signals)
{
	CALLED();
	bigtime_t time;
	status_t err;
	do {
		time = RealTimeFor(performance_time, with_latency);
		err = snooze_until(time, B_SYSTEM_TIMEBASE);
	} while (err == B_INTERRUPTED && retry_signals);
	return err;
}


bigtime_t
BTimeSource::Now()
{
	PRINT(8, "CALLED BTimeSource::Now()\n");
	return PerformanceTimeFor(RealTime());
}


bigtime_t
BTimeSource::PerformanceTimeFor(bigtime_t real_time)
{
	PRINT(8, "CALLED BTimeSource::PerformanceTimeFor()\n");
	bigtime_t last_perf_time;
	bigtime_t last_real_time;
	float last_drift;

	if (GetTime(&last_perf_time, &last_real_time, &last_drift) != B_OK)
		debugger("BTimeSource::PerformanceTimeFor: GetTime failed");

	return last_perf_time
		+ (bigtime_t)((real_time - last_real_time) * last_drift);
}


bigtime_t
BTimeSource::RealTimeFor(bigtime_t performance_time,
	bigtime_t with_latency)
{
	PRINT(8, "CALLED BTimeSource::RealTimeFor()\n");

	if (fIsRealtime) {
		return performance_time - with_latency;
	}

	bigtime_t last_perf_time;
	bigtime_t last_real_time;
	float last_drift;

	if (GetTime(&last_perf_time, &last_real_time, &last_drift) != B_OK)
		debugger("BTimeSource::RealTimeFor: GetTime failed");

	return last_real_time - with_latency
		+ (bigtime_t)((performance_time - last_perf_time) / last_drift);
}


bool
BTimeSource::IsRunning()
{
	PRINT(8, "CALLED BTimeSource::IsRunning()\n");

	bool isrunning;

	// The system time source is always running
	if (fIsRealtime)
		isrunning = true;
	else
		isrunning = fBuf ? atomic_add(&fBuf->isrunning, 0) : fStarted;

	TRACE_TIMESOURCE("BTimeSource::IsRunning() node %" B_PRId32 ", port %"
		B_PRId32 ", %s\n", fNodeID, fControlPort, isrunning ? "yes" : "no");
	return isrunning;
}


status_t
BTimeSource::GetTime(bigtime_t* performance_time,
	bigtime_t* real_time, float* drift)
{
	PRINT(8, "CALLED BTimeSource::GetTime()\n");

	if (fIsRealtime) {
		*performance_time = *real_time = system_time();
		*drift = 1.0f;
		return B_OK;
	}

	if (fBuf == NULL)
		debugger("BTimeSource::GetTime: fBuf == NULL");

	int32 index = _atomic_read(&fBuf->readindex);
	index &= (TS_INDEX_COUNT - 1);
	*real_time = fBuf->realtime[index];
	*performance_time = fBuf->perftime[index];
	*drift = fBuf->drift[index];

	TRACE_TIMESOURCE("BTimeSource::GetTime     timesource %" B_PRId32
		", perf %16" B_PRId64 ", real %16" B_PRId64 ", drift %2.2f\n", ID(),
		*performance_time, *real_time, *drift);
	return B_OK;
}


bigtime_t
BTimeSource::RealTime()
{
	PRINT(8, "CALLED BTimeSource::RealTime()\n");
	return system_time();
}


status_t
BTimeSource::GetStartLatency(bigtime_t* out_latency)
{
	CALLED();
	*out_latency = 0;
	return B_OK;
}

/*************************************************************
 * protected BTimeSource
 *************************************************************/


BTimeSource::BTimeSource()
	:
	BMediaNode("This one is never called"),
	fStarted(false),
	fArea(-1),
	fBuf(NULL),
	fSlaveNodes(new BPrivate::media::SlaveNodes),
	fIsRealtime(false)
{
	CALLED();
	AddNodeKind(B_TIME_SOURCE);
	// This constructor is only called by real time sources that inherit
	// BTimeSource. We create the communication area in FinishCreate(),
	// since we don't have a correct ID() until this node is registered.
}


status_t
BTimeSource::HandleMessage(int32 message, const void* rawdata,
	size_t size)
{
	PRINT(4, "BTimeSource::HandleMessage %#" B_PRIx32 ", node %" B_PRId32 "\n",
		message, fNodeID);
	status_t rv;
	switch (message) {
		case TIMESOURCE_OP:
		{
			const time_source_op_info* data
				= static_cast<const time_source_op_info*>(rawdata);

			status_t result;
			result = TimeSourceOp(*data, NULL);
			if (result != B_OK) {
				ERROR("BTimeSource::HandleMessage: TimeSourceOp failed\n");
			}

			switch (data->op) {
				case B_TIMESOURCE_START:
					DirectStart(data->real_time);
					break;
				case B_TIMESOURCE_STOP:
					DirectStop(data->real_time, false);
					break;
				case B_TIMESOURCE_STOP_IMMEDIATELY:
					DirectStop(data->real_time, true);
					break;
				case B_TIMESOURCE_SEEK:
					DirectSeek(data->performance_time, data->real_time);
					break;
			}
			return B_OK;
		}

		case TIMESOURCE_ADD_SLAVE_NODE:
		{
			const timesource_add_slave_node_command* data
				= static_cast<const timesource_add_slave_node_command*>(rawdata);
			DirectAddMe(data->node);
			return B_OK;
		}

		case TIMESOURCE_REMOVE_SLAVE_NODE:
		{
			const timesource_remove_slave_node_command* data
				= static_cast<const timesource_remove_slave_node_command*>(rawdata);
			DirectRemoveMe(data->node);
			return B_OK;
		}

		case TIMESOURCE_GET_START_LATENCY:
		{
			const timesource_get_start_latency_request* request
				= static_cast<const timesource_get_start_latency_request*>(rawdata);
			timesource_get_start_latency_reply reply;
			rv = GetStartLatency(&reply.start_latency);
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}
	}
	return B_ERROR;
}


void
BTimeSource::PublishTime(bigtime_t performance_time,
	bigtime_t real_time, float drift)
{
	TRACE_TIMESOURCE("BTimeSource::PublishTime timesource %" B_PRId32
		", perf %16" B_PRId64 ", real %16" B_PRId64 ", drift %2.2f\n", ID(),
		performance_time, real_time, drift);
	if (0 == fBuf) {
		ERROR("BTimeSource::PublishTime timesource %" B_PRId32
			", fBuf = NULL\n", ID());
		fStarted = true;
		return;
	}

	int32 index = atomic_add(&fBuf->writeindex, 1);
	index &= (TS_INDEX_COUNT - 1);
	fBuf->realtime[index] = real_time;
	fBuf->perftime[index] = performance_time;
	fBuf->drift[index] = drift;
	atomic_add(&fBuf->readindex, 1);
}


void
BTimeSource::BroadcastTimeWarp(bigtime_t at_real_time,
	bigtime_t new_performance_time)
{
	CALLED();
	ASSERT(fSlaveNodes != NULL);

	// calls BMediaNode::TimeWarp() of all slaved nodes

	TRACE("BTimeSource::BroadcastTimeWarp: at_real_time %" B_PRId64
		", new_performance_time %" B_PRId64 "\n", at_real_time,
		new_performance_time);

	BAutolock lock(fSlaveNodes);

	port_id* port = NULL;
	while (fSlaveNodes->GetNextSlave(&port) == true) {
		node_time_warp_command cmd;
		cmd.at_real_time = at_real_time;
		cmd.to_performance_time = new_performance_time;
		SendToPort(*port, NODE_TIME_WARP,
			&cmd, sizeof(cmd));
	}
	fSlaveNodes->Rewind();
}


void
BTimeSource::SendRunMode(run_mode mode)
{
	CALLED();
	ASSERT(fSlaveNodes != NULL);

	// send the run mode change to all slaved nodes

	BAutolock lock(fSlaveNodes);

	port_id* port = NULL;
	while (fSlaveNodes->GetNextSlave(&port) == true) {
		node_set_run_mode_command cmd;
		cmd.mode = mode;
		SendToPort(*port, NODE_SET_RUN_MODE,
			&cmd, sizeof(cmd));
	}
	fSlaveNodes->Rewind();
}


void
BTimeSource::SetRunMode(run_mode mode)
{
	CALLED();
	BMediaNode::SetRunMode(mode);
	SendRunMode(mode);
}
/*************************************************************
 * private BTimeSource
 *************************************************************/

/*
//unimplemented
BTimeSource::BTimeSource(const BTimeSource &clone)
BTimeSource &BTimeSource::operator=(const BTimeSource &clone)
*/

status_t BTimeSource::_Reserved_TimeSource_0(void *) { return B_ERROR; }
status_t BTimeSource::_Reserved_TimeSource_1(void *) { return B_ERROR; }
status_t BTimeSource::_Reserved_TimeSource_2(void *) { return B_ERROR; }
status_t BTimeSource::_Reserved_TimeSource_3(void *) { return B_ERROR; }
status_t BTimeSource::_Reserved_TimeSource_4(void *) { return B_ERROR; }
status_t BTimeSource::_Reserved_TimeSource_5(void *) { return B_ERROR; }

/* explicit */
BTimeSource::BTimeSource(media_node_id id)
	:
	BMediaNode("This one is never called"),
	fStarted(false),
	fArea(-1),
	fBuf(NULL),
	fSlaveNodes(NULL),
	fIsRealtime(false)
{
	CALLED();
	AddNodeKind(B_TIME_SOURCE);
	ASSERT(id > 0);

	// This constructor is only called by the derived
	// BPrivate::media::TimeSourceObject objects
	// We create a clone of the communication area.
	char name[32];
	sprintf(name, "__timesource_buf_%" B_PRId32, id);
	area_id area = find_area(name);
	if (area <= 0) {
		ERROR("BTimeSource::BTimeSource couldn't find area, node %" B_PRId32
			"\n", id);
		return;
	}
	sprintf(name, "__cloned_timesource_buf_%" B_PRId32, id);

	void** buf = reinterpret_cast<void**>
		(const_cast<BPrivate::media::TimeSourceTransmit**>(&fBuf));

	fArea = clone_area(name, buf, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, area);

	if (fArea <= 0) {
		ERROR("BTimeSource::BTimeSource couldn't clone area, node %" B_PRId32
			"\n", id);
		return;
	}
}


void
BTimeSource::FinishCreate()
{
	CALLED();

	char name[32];
	sprintf(name, "__timesource_buf_%" B_PRId32, ID());

	void** buf = reinterpret_cast<void**>
		(const_cast<BPrivate::media::TimeSourceTransmit**>(&fBuf));

	fArea = create_area(name, buf, B_ANY_ADDRESS, TS_AREA_SIZE,
		B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA);

	if (fArea <= 0) {
		ERROR("BTimeSource::BTimeSource couldn't create area, node %" B_PRId32
			"\n", ID());
		fBuf = NULL;
		return;
	}
	fBuf->readindex = 0;
	fBuf->writeindex = 1;
	fBuf->realtime[0] = 0;
	fBuf->perftime[0] = 0;
	fBuf->drift[0] = 1.0f;
	fBuf->isrunning = fStarted;
}


status_t
BTimeSource::RemoveMe(BMediaNode* node)
{
	CALLED();
	if (fKinds & NODE_KIND_SHADOW_TIMESOURCE) {
		timesource_remove_slave_node_command cmd;
		cmd.node = node->Node();
		SendToPort(fControlPort, TIMESOURCE_REMOVE_SLAVE_NODE,
			&cmd, sizeof(cmd));
	} else {
		DirectRemoveMe(node->Node());
	}
	return B_OK;
}


status_t
BTimeSource::AddMe(BMediaNode* node)
{
	CALLED();
	if (fKinds & NODE_KIND_SHADOW_TIMESOURCE) {
		timesource_add_slave_node_command cmd;
		cmd.node = node->Node();
		SendToPort(fControlPort, TIMESOURCE_ADD_SLAVE_NODE, &cmd, sizeof(cmd));
	} else {
		DirectAddMe(node->Node());
	}
	return B_OK;
}


void
BTimeSource::DirectAddMe(const media_node& node)
{
	CALLED();
	ASSERT(fSlaveNodes != NULL);
	BAutolock lock(fSlaveNodes);

	if (fSlaveNodes->CountSlaves() == MAX_SLAVE_NODES) {
		ERROR("BTimeSource::DirectAddMe reached maximum number of slaves\n");
		return;
	}
	if (fNodeID == node.node) {
		ERROR("BTimeSource::DirectAddMe should not add itself to slave nodes\n");
		return;
	}

	if (fSlaveNodes->InsertSlave(node) != true) {
		ERROR("BTimeSource::DirectAddMe failed\n");
		return;
	}

	if (fSlaveNodes->CountSlaves() == 1) {
		// start the time source
		time_source_op_info msg;
		msg.op = B_TIMESOURCE_START;
		msg.real_time = RealTime();

		TRACE_TIMESOURCE("starting time source %" B_PRId32 "\n", ID());

		write_port(fControlPort, TIMESOURCE_OP, &msg, sizeof(msg));
	}
 }


void
BTimeSource::DirectRemoveMe(const media_node& node)
{
	CALLED();
	ASSERT(fSlaveNodes != NULL);
	BAutolock lock(fSlaveNodes);

	if (fSlaveNodes->CountSlaves() == 0) {
		ERROR("BTimeSource::DirectRemoveMe no slots used\n");
		return;
	}

	if (fSlaveNodes->RemoveSlave(node) != true) {
		ERROR("BTimeSource::DirectRemoveMe failed\n");
		return;
	}

	if (fSlaveNodes->CountSlaves() == 0) {
		// stop the time source
		time_source_op_info msg;
		msg.op = B_TIMESOURCE_STOP_IMMEDIATELY;
		msg.real_time = RealTime();

		TRACE_TIMESOURCE("stopping time source %" B_PRId32 "\n", ID());

		write_port(fControlPort, TIMESOURCE_OP, &msg, sizeof(msg));
	}
}


void
BTimeSource::DirectStart(bigtime_t at)
{
	CALLED();
	if (fBuf)
		atomic_or(&fBuf->isrunning, 1);
	else
		fStarted = true;
}


void
BTimeSource::DirectStop(bigtime_t at, bool immediate)
{
	CALLED();
	if (fBuf)
		atomic_and(&fBuf->isrunning, 0);
	else
		fStarted = false;
}


void
BTimeSource::DirectSeek(bigtime_t to, bigtime_t at)
{
	UNIMPLEMENTED();
}


void
BTimeSource::DirectSetRunMode(run_mode mode)
{
	UNIMPLEMENTED();
}
