/*
 * Copyright 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 */
#ifndef TIME_SOURCE_OBJECT_H
#define TIME_SOURCE_OBJECT_H


#include <TimeSource.h>

#include <MediaMisc.h>


namespace BPrivate {
namespace media {


class TimeSourceObject : public BTimeSource {
public:
								TimeSourceObject(const media_node& node);

protected:
	virtual	status_t			TimeSourceOp(const time_source_op_info& op,
									void* _reserved);

	virtual	BMediaAddOn*		AddOn(int32* _id) const;

	// override from BMediaNode
	virtual status_t			DeleteHook(BMediaNode* node);
};


}	// namespace media
}	// namespace BPrivate


using namespace BPrivate::media;


#endif	// TIME_SOURCE_OBJECT_H
