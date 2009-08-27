/*
 * Copyright 2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TIME_SOURCE_H
#define _TIME_SOURCE_H


#include <MediaDefs.h>
#include <MediaNode.h>


class _BSlaveNodeStorageP;

namespace BPrivate {
	namespace media {
		class BMediaRosterEx;
		class TimeSourceObject;
		class SystemTimeSourceObject;
		class SlaveNodes;
		struct TimeSourceTransmit;
	}
}


class BTimeSource : public virtual BMediaNode {
protected:
	virtual						~BTimeSource();

public:
	virtual	status_t			SnoozeUntil(bigtime_t performanceTime,
									bigtime_t withLatency = 0,
									bool retrySignals = false);

			bigtime_t			Now();
			bigtime_t			PerformanceTimeFor(bigtime_t realTime);
			bigtime_t			RealTimeFor(bigtime_t performanceTime,
									bigtime_t withLatency);
			bool				IsRunning();

			status_t			GetTime(bigtime_t* _performanceTime,
									bigtime_t* _realTime, float* _drift);
			status_t			GetStartLatency(bigtime_t* _latency);

	static	bigtime_t			RealTime();

protected:
								BTimeSource();

	virtual	status_t			HandleMessage(int32 message, const void* data,
									size_t size);

			void				PublishTime(bigtime_t performanceTime,
									bigtime_t realTime, float drift);

			void				BroadcastTimeWarp(bigtime_t atRealTime,
									bigtime_t newPerformanceTime);

			void				SendRunMode(run_mode mode);
	virtual	void				SetRunMode(run_mode mode);

	enum time_source_op {
		B_TIMESOURCE_START = 1,
		B_TIMESOURCE_STOP,
		B_TIMESOURCE_STOP_IMMEDIATELY,
		B_TIMESOURCE_SEEK
	};

	struct time_source_op_info {
		time_source_op	op;
		int32			_reserved1;
		bigtime_t		real_time;
		bigtime_t		performance_time;
		int32			_reserved2[6];
	};

	virtual	status_t			TimeSourceOp(const time_source_op_info& op,
									void* _reserved) = 0;

private:
	friend class BMediaNode;
	friend class BMediaRoster;
	friend class BPrivate::media::BMediaRosterEx;
	friend class BPrivate::media::SystemTimeSourceObject;
	friend class BPrivate::media::TimeSourceObject;

								BTimeSource(const BTimeSource& other);
			BTimeSource&		operator=(const BTimeSource& other);
									// not implemented

	explicit					BTimeSource(media_node_id id);

			status_t			_Reserved_TimeSource_0(void*);
				// used for TimeSourceOp()
	virtual	status_t			_Reserved_TimeSource_1(void*);
	virtual	status_t			_Reserved_TimeSource_2(void*);
	virtual	status_t			_Reserved_TimeSource_3(void*);
	virtual	status_t			_Reserved_TimeSource_4(void*);
	virtual	status_t			_Reserved_TimeSource_5(void*);

	virtual	status_t			RemoveMe(BMediaNode* node);
	virtual	status_t			AddMe(BMediaNode* node);

			void				FinishCreate();

			void				DirectStart(bigtime_t at);
			void				DirectStop(bigtime_t at, bool immediate);
			void				DirectSeek(bigtime_t to, bigtime_t at);
			void				DirectSetRunMode(run_mode mode);
			void				DirectAddMe(const media_node& node);
			void				DirectRemoveMe(const media_node& node);

private:
			bool				fStarted;
			area_id				fArea;
	volatile BPrivate::media::TimeSourceTransmit* fBuf;
			BPrivate::media::SlaveNodes* fSlaveNodes;

			area_id				_reserved_area;
			bool				fIsRealtime;
			bool				_reserved_bool_[3];
			uint32				_reserved_time_source_[10];
};


#endif	// _TIME_SOURCE_H
