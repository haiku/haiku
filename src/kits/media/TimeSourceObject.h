/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the OpenBeOS License.
 *
 * The object returned by BMediaRoster's
 * BTimeSource * MakeTimeSourceFor(const media_node & for_node);
 *
 ***********************************************************************/
#ifndef _TIME_SOURCE_OBJECT_H_
#define _TIME_SOURCE_OBJECT_H_

#include <TimeSource.h>

namespace BPrivate { namespace media {

class TimeSourceObject : public BTimeSource
{
public:
	TimeSourceObject(const media_node &node);
	
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

} } using namespace BPrivate::media;

#endif
