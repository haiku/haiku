/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: TimeSource.cpp
 *  DESCR: 
 ***********************************************************************/
#include <TimeSource.h>
#include "debug.h"
#include "../server/headers/ServerInterface.h"

// XXX This BTimeSource only works for realtime, nothing else is implemented

// XXX The bebook says that the latency is always calculated in realtime
// XXX This is not currently done in this code

/*************************************************************
 * protected BTimeSource
 *************************************************************/

BTimeSource::~BTimeSource()
{
	CALLED();
}

/*************************************************************
 * public BTimeSource
 *************************************************************/

status_t
BTimeSource::SnoozeUntil(bigtime_t performance_time,
						 bigtime_t with_latency,
						 bool retry_signals)
{
	CALLED();

	return B_ERROR;
}


bigtime_t
BTimeSource::Now()
{
	CALLED();
	return PerformanceTimeFor(RealTime());
}


bigtime_t
BTimeSource::PerformanceTimeFor(bigtime_t real_time)
{
	CALLED();
	bigtime_t performanceTime; 
	float drift; 

	while (GetTime(&performanceTime, &real_time, &drift) != B_OK)
		snooze(1000);
		
	return (bigtime_t)(performanceTime + (RealTime() - real_time) * drift);
}


bigtime_t
BTimeSource::RealTimeFor(bigtime_t performance_time,
						 bigtime_t with_latency)
{
	CALLED();

	return performance_time - with_latency;
}


bool
BTimeSource::IsRunning()
{
	CALLED();
	return !fStopped;
}


status_t
BTimeSource::GetTime(bigtime_t *performance_time,
					 bigtime_t *real_time,
					 float *drift)
{
	CALLED();
	*performance_time = *real_time;
	*drift = 1.0f;

	return B_OK;
}


bigtime_t
BTimeSource::RealTime()
{
	CALLED();
	return system_time();
}


status_t
BTimeSource::GetStartLatency(bigtime_t *out_latency)
{
	CALLED();
	*out_latency = 0;
	return B_OK;
}

/*************************************************************
 * protected BTimeSource
 *************************************************************/


BTimeSource::BTimeSource() : 
	BMediaNode("called by BTimeSource"),
	fStopped(false)
{
	CALLED();

	AddNodeKind(B_TIME_SOURCE);
}


status_t
BTimeSource::HandleMessage(int32 message,
						   const void *rawdata,
						   size_t size)
{
	CALLED();
	switch (message) {
		case TIMESOURCE_OP:
		{
			const time_source_op_info *data = (const time_source_op_info *)rawdata;
			status_t result;
			result = TimeSourceOp(*data, NULL);
			return B_OK;
		}
	};
	return B_ERROR;
}


void
BTimeSource::PublishTime(bigtime_t performance_time,
						 bigtime_t real_time,
						 float drift)
{
	UNIMPLEMENTED();
}


void
BTimeSource::BroadcastTimeWarp(bigtime_t at_real_time,
							   bigtime_t new_performance_time)
{
	UNIMPLEMENTED();
}


void
BTimeSource::SendRunMode(run_mode mode)
{
	UNIMPLEMENTED();
}


void
BTimeSource::SetRunMode(run_mode mode)
{
	CALLED();
	BMediaNode::SetRunMode(mode);
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
BTimeSource::BTimeSource(media_node_id id) :
	BMediaNode("called by BTimeSource", id, 0),
	fStopped(false)
{
	CALLED();

	AddNodeKind(B_TIME_SOURCE);
}


void
BTimeSource::FinishCreate()
{
	UNIMPLEMENTED();
}


status_t
BTimeSource::RemoveMe(BMediaNode *node)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BTimeSource::AddMe(BMediaNode *node)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


void
BTimeSource::DirectStart(bigtime_t at)
{
	UNIMPLEMENTED();
}


void
BTimeSource::DirectStop(bigtime_t at,
						bool immediate)
{
	UNIMPLEMENTED();
}


void
BTimeSource::DirectSeek(bigtime_t to,
						bigtime_t at)
{
	UNIMPLEMENTED();
}


void
BTimeSource::DirectSetRunMode(run_mode mode)
{
	UNIMPLEMENTED();
}


