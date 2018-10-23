/*
 * Copyright 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 */


/*!	This works like a cache for time source objects, to make sure
	each team only has one object representation for each time source.
*/


#include "TimeSourceObjectManager.h"

#include <stdio.h>

#include <Autolock.h>
#include <MediaRoster.h>

#include <MediaDebug.h>
#include <MediaMisc.h>

#include "TimeSourceObject.h"


namespace BPrivate {
namespace media {


TimeSourceObjectManager* gTimeSourceObjectManager;
	// initialized by BMediaRoster.


TimeSourceObjectManager::TimeSourceObjectManager()
	:
	BLocker("time source object manager")
{
}


TimeSourceObjectManager::~TimeSourceObjectManager()
{
	CALLED();

	// force unloading all currently loaded time sources
	NodeMap::iterator iterator = fMap.begin();
	for (; iterator != fMap.end(); iterator++) {
		BTimeSource* timeSource = iterator->second;

		PRINT(1, "Forcing release of TimeSource id %ld...\n", timeSource->ID());
		int32 debugCount = 0;
		while (timeSource->Release() != NULL)
			debugCount++;

		PRINT(1, "Forcing release of TimeSource done, released %d times\n",
			debugCount);
	}
}


/*!	BMediaRoster::MakeTimeSourceFor does use this function to request
	a time source object. If it is already in memory, it will be
	Acquired(), if not, a new TimeSourceObject will be created.
*/
BTimeSource*
TimeSourceObjectManager::GetTimeSource(const media_node& node)
{
	CALLED();
	BAutolock _(this);

	PRINT(1, "TimeSourceObjectManager::GetTimeSource, node id %ld\n",
		node.node);

	NodeMap::iterator found = fMap.find(node.node);
	if (found != fMap.end())
		return dynamic_cast<BTimeSource*>(found->second->Acquire());

	// time sources are not accounted in node reference counting
	BTimeSource* timeSource = new(std::nothrow) TimeSourceObject(node);
	if (timeSource == NULL)
		return NULL;

	fMap.insert(std::make_pair(node.node, timeSource));
	return timeSource;
}


/*!	This function is called during deletion of the time source object.
*/
void
TimeSourceObjectManager::ObjectDeleted(BTimeSource* timeSource)
{
	CALLED();
	BAutolock _(this);

	PRINT(1, "TimeSourceObjectManager::ObjectDeleted, node id %ld\n",
		timeSource->ID());

	fMap.erase(timeSource->ID());

	status_t status = BMediaRoster::Roster()->ReleaseNode(timeSource->Node());
	if (status != B_OK) {
		ERROR("TimeSourceObjectManager::ObjectDeleted, ReleaseNode failed\n");
	}
}


}	// namespace media
}	// namespace BPrivate
