/*
 * Copyright 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 */
#ifndef TIME_SOURCE_OBJECT_MANAGER_H
#define TIME_SOURCE_OBJECT_MANAGER_H


#include <map>

#include <Locker.h>
#include <MediaDefs.h>


class BTimeSource;


namespace BPrivate {
namespace media {


class TimeSourceObjectManager : BLocker {
public:
								TimeSourceObjectManager();
								~TimeSourceObjectManager();

			BTimeSource*		GetTimeSource(const media_node& node);
			void				ObjectDeleted(BTimeSource* timeSource);

private:
			typedef std::map<media_node_id, BTimeSource*> NodeMap;

			NodeMap				fMap;
};


extern TimeSourceObjectManager* gTimeSourceObjectManager;


}	// namespace media
}	// namespace BPrivate


using BPrivate::media::gTimeSourceObjectManager;


#endif	// _TIME_SOURCE_OBJECT_MANAGER_H_
