/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the OpenBeOS License.
 *
 * The realtime BTimeSource
 ***********************************************************************/

// XXX This works only as long a BTimeSource is only supporting realtime

#include <OS.h>
#include <stdio.h>
#include <MediaRoster.h>
#include "SystemTimeSource.h"

_SysTimeSource::_SysTimeSource() :
	BMediaNode("system time source")
{
	printf("_SysTimeSource::_SysTimeSource enter\n");
	BMediaRoster::Roster()->RegisterNode(this); // XXX
	printf("_SysTimeSource::_SysTimeSource leave\n");
}

/* virtual */ status_t
_SysTimeSource::SnoozeUntil(
				bigtime_t performance_time,
				bigtime_t with_latency,
				bool retry_signals)
{
	bigtime_t time = performance_time - with_latency;
	status_t err;
	do {
		err = snooze_until(time, B_SYSTEM_TIMEBASE);
	} while (err == B_INTERRUPTED && retry_signals);
	return err;
}

/* virtual */ status_t
_SysTimeSource::TimeSourceOp(
				const time_source_op_info & op,
				void * _reserved)
{
	return B_OK;
}

/* virtual */ BMediaAddOn* 
_SysTimeSource::AddOn(int32 * internal_id) const
{
	return NULL;
}
