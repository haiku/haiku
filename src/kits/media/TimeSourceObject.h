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
	TimeSourceObject(media_node_id id);
	
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
	SystemTimeSourceObject(media_node_id id);

protected:
	// override from BMediaNode				
	virtual status_t DeleteHook(BMediaNode * node);

};

} } using namespace BPrivate::media;

#endif
