/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 *
 * This works like a cache for time source objects, to make sure
 * each team only has one object representation for each time source.
 *
 ***********************************************************************/

#include <OS.h>
#include <stdio.h>
#include <MediaRoster.h>
#include <Autolock.h>
#include "TimeSourceObjectManager.h"
#include "TimeSourceObject.h"
#include "MediaMisc.h"
#include "debug.h"


static BPrivate::media::TimeSourceObjectManager manager;
BPrivate::media::TimeSourceObjectManager *_TimeSourceObjectManager = &manager;

namespace BPrivate {
namespace media {

TimeSourceObjectManager::TimeSourceObjectManager()
 :	fSystemTimeSource(0)
{
	CALLED();
	fLock = new BLocker("timesource object manager locker");
	fMap = new Map<media_node_id,BTimeSource *>;
}


TimeSourceObjectManager::~TimeSourceObjectManager()
{
	CALLED();
	delete fLock;

	// force unloading all currently loaded 
	BTimeSource **pts;
	for (fMap->Rewind(); fMap->GetNext(&pts); ) {
		FATAL("Forcing release of TimeSource id %ld...\n", (*pts)->ID());
		int debugcnt = 0;
		while ((*pts)->Release() != NULL)
			debugcnt++;
		FATAL("Forcing release of TimeSource done, released %d times\n", debugcnt);
	}
	
	delete fMap;
}

void
TimeSourceObjectManager::InitSystemTimeSource()
{
	CALLED();
	BAutolock lock(fLock);

	if (fSystemTimeSource != 0)
		return;

	TRACE("TimeSourceObjectManager::InitSystemTimeSource enter\n");
	
	media_node node;
	node.node = NODE_SYSTEM_TIMESOURCE_ID;
	node.port = SYSTEM_TIMESOURCE_CONTROL_PORT;
	node.kind = B_TIME_SOURCE;
	
	fSystemTimeSource = new SystemTimeSourceObject(node);

	TRACE("TimeSourceObjectManager::InitSystemTimeSource leave\n");
}


BTimeSource *
TimeSourceObjectManager::GetSystemTimeSource()
{
	CALLED();
	BAutolock lock(fLock);

	if (fSystemTimeSource == 0)
		InitSystemTimeSource();

	return dynamic_cast<BTimeSource *>(fSystemTimeSource->Acquire());
}

/* BMediaRoster::MakeTimeSourceFor does use this function to request
 * a time source object. If it is already in memory, it will be
 * Acquired(), if not, a new TimeSourceObject will be created.
 */
BTimeSource *
TimeSourceObjectManager::GetTimeSource(const media_node &node)
{
	CALLED();
	BAutolock lock(fLock);

//	printf("TimeSourceObjectManager::GetTimeSource, node id %ld\n", node.node);
	
	if (fSystemTimeSource == 0)
		InitSystemTimeSource();
	
	if (NODE_SYSTEM_TIMESOURCE_ID == node.node)
		return dynamic_cast<BTimeSource *>(fSystemTimeSource->Acquire());
	
	BTimeSource **pts;
	if (fMap->Get(node.node, &pts))
		return dynamic_cast<BTimeSource *>((*pts)->Acquire());

/*		
	media_node clone;
	status_t rv;
	
	rv = BMediaRoster::Roster()->GetNodeFor(node.node, &clone);
	if (rv != B_OK) {
		FATAL("TimeSourceObjectManager::GetTimeSource, GetNodeFor %ld failed\n", node.node);
		return NULL;
	}
		
	BTimeSource *ts;
	ts = new TimeSourceObject(clone);
	fMap->Insert(clone.node, ts);
	return ts;
*/
	// time sources are not accounted in node reference counting
	BTimeSource *ts;
	ts = new TimeSourceObject(node);
	fMap->Insert(node.node, ts);
	return ts;
}

/* This function is called during deletion of the time source object.
 *
 * I'm not sure if there is a race condition with the function above.
 */
void
TimeSourceObjectManager::ObjectDeleted(BTimeSource *timesource)
{
	CALLED();
	BAutolock lock(fLock);

//	printf("TimeSourceObjectManager::ObjectDeleted, node id %ld\n", timesource->ID());
	
	bool b;
	b = fMap->Remove(timesource->ID());
	if (!b) {
		FATAL("TimeSourceObjectManager::ObjectDeleted, Remove failed\n");
	}
	
	status_t rv;
	rv = BMediaRoster::Roster()->ReleaseNode(timesource->Node());
	if (rv != B_OK) {
		FATAL("TimeSourceObjectManager::ObjectDeleted, ReleaseNode failed\n");
	}
}

}; // namespace media
}; // namespace BPrivate
