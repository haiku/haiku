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

#define NODE_KIND_USER_MASK 			0x00000000FFFFFFFFLL
#define NODE_KIND_SHADOW_TIMESOURCE		0x0000000100000000LL
#define NODE_KIND_SYSTEM_TIMESOURCE		0x0000000200000000LL

namespace BPrivate { namespace media {

class TimeSourceObject : public BTimeSource
{
public:
	TimeSourceObject(const media_node &node);
	
protected:
	virtual	status_t TimeSourceOp(
				const time_source_op_info & op,
				void * _reserved);

	virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const;

	// override from BMediaNode				
	virtual status_t DeleteHook(BMediaNode * node);
};

class SystemTimeSourceObject : public TimeSourceObject
{
public:
	SystemTimeSourceObject(const media_node &node);

protected:
	// override from BMediaNode				
	virtual status_t DeleteHook(BMediaNode * node);

};

} } using namespace BPrivate::media;

#endif
