/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the OpenBeOS License.
 *
 * The object returned by BMediaRoster's
 * BTimeSource * MakeTimeSourceFor(const media_node & for_node);
 *
 ***********************************************************************/

#include <OS.h>
#include <stdio.h>
#include <MediaRoster.h>
#include "TimeSourceObject.h"

TimeSourceObject::TimeSourceObject(const media_node &node) :
	BMediaNode("some timesource object")
{
	printf("TimeSourceObject::TimeSourceObject enter\n");
	delete_port(fControlPort);
	fControlPort = -999666;
	printf("TimeSourceObject::TimeSourceObject leave\n");
}

/* virtual */ status_t
TimeSourceObject::SnoozeUntil(
				bigtime_t performance_time,
				bigtime_t with_latency,
				bool retry_signals)
{
	return B_ERROR;
}

/* virtual */ status_t
TimeSourceObject::TimeSourceOp(
				const time_source_op_info & op,
				void * _reserved)
{
	return B_OK;
}

/* virtual */ BMediaAddOn* 
TimeSourceObject::AddOn(int32 * internal_id) const
{
	return NULL;
}
