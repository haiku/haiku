/*
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 */
#ifndef _TIME_SOURCE_OBJECT_MANAGER_H_
#define _TIME_SOURCE_OBJECT_MANAGER_H_


#include "TMap.h"

class BLocker;


namespace BPrivate {
namespace media {

class TimeSourceObjectManager {
public:
	TimeSourceObjectManager();
	~TimeSourceObjectManager();

	BTimeSource *GetTimeSource(const media_node &node);
	void ObjectDeleted(BTimeSource *timesource);

private:
	Map<media_node_id, BTimeSource *> *fMap;
	BLocker *fLock;
};

} // namespace media
} // namespace BPrivate

extern BPrivate::media::TimeSourceObjectManager *_TimeSourceObjectManager;

#endif	// _TIME_SOURCE_OBJECT_MANAGER_H_
