/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the OpenBeOS License.
 *
 * The realtime BTimeSource
 ***********************************************************************/
#ifndef _SYSTEM_TIME_SOURCE_H_
#define _SYSTEM_TIME_SOURCE_H_

#include <TimeSource.h>

class _SysTimeSource : public BTimeSource
{
public:
	_SysTimeSource();
	
	virtual	status_t SnoozeUntil(
				bigtime_t performance_time,
				bigtime_t with_latency = 0,
				bool retry_signals = false);
protected:
	virtual	status_t TimeSourceOp(
				const time_source_op_info & op,
				void * _reserved);

virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const;

};

#endif
